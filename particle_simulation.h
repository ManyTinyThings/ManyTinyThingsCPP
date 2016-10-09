#ifndef particle_simulation_h
#define particle_simulation_h

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "math_stuff.h"
#include "types.h"
 

struct Particle {
	V2 position;
	V2 velocity;
	V2 acceleration;

	f64 mass;
	f64 radius;
	Color4 color;

	int gridCol;
	int gridRow;

	// measurements
	f64 potentialEnergy;
	f64 kineticEnergy;

};

struct Wall {
	V2 start;
	V2 end;
};

struct Simulation {
	Particle* particles;
	int particleCount;

	// box
	f64 boxWidth;
	f64 boxHeight;

	Particle** particleGrid;
	int gridRowCount;
	int gridColCount;
	f64 gridCellWidth;
	f64 gridCellHeight;

	// walls
	Wall* walls;
	int wallCount;
	// TODO: implement wallstrength
	f64 wallStrength;

	// time
	f64 dt;
	f64 timeLeftToSimulate;

	// TODO: have different particle types and corresponding interactions

	// interactions
	f64 separation;
	f64 bondEnergy;
	f64 cutoffFactor;
    f64 gravityStrength;

	// thermostat
	f32 temperature;
	f32 viscosity;

	// user interaction
	bool isDragging;
	V2 mousePosition;
	int draggedParticleIndex;
	f64 draggingStrength;
};

int
pickParticle(Simulation* simulation, V2 pickPosition)
{
	for (int particleIndex = 0; particleIndex < simulation->particleCount; ++particleIndex)
	{
		Particle* particle = simulation->particles + particleIndex;
		V2 relativePosition = pickPosition - particle->position;
		if (square(relativePosition) < square(particle->radius))
		{
			return particleIndex;
		}
	}
	return -1;
}

V2
hexagonLatticePosition(int particleIndex)
{
    if (particleIndex == 0)
    {
        return v2(0, 0);
    }

    f64 k = particleIndex - 1;
    u64 layer = floor((sqrt(8 * (k / 6) + 1) - 1) / 2) + 1;
    u64 rest = k - 6 * layer * (layer - 1) / 2;
    u64 triangleIndex = floor( (f64) rest / layer);
    f64 x = layer;
    f64 y = rest % layer;
    f64 angle = triangleIndex * tau / 6;
    V2 latticeX = v2FromAngle(angle);
    V2 latticeY = v2FromAngle(angle + tau / 3);
    return (x * latticeX + y * latticeY);
}

void
setParticleCount(Simulation* simulation, int particleCount)
{
	assert(particleCount < (simulation->gridColCount * simulation->gridRowCount));

	if (particleCount > simulation->particleCount)
	{
		Particle* newPointer = (Particle*)realloc(simulation->particles, particleCount * sizeof(Particle));
		if (!newPointer) return;
		simulation->particles = newPointer;
		int newParticleCount = particleCount - simulation->particleCount;
		memset(simulation->particles + simulation->particleCount, 0, newParticleCount * sizeof(Particle));
	}
	for (int particleIndex = simulation->particleCount; particleIndex < particleCount; ++particleIndex)
	{
		// set defaults
		Particle* particle = simulation->particles + particleIndex;
		particle->mass = 1;
		particle->radius = 1;
		particle->color = c4(0, 0, 0, 1);
	}
	simulation->particleCount = particleCount;
}

void
updateGrid(Simulation* simulation)
{
	// TODO: figure out grid size from interaction

	f64 radius = 1;
	f64 maxCellSide = radius;
	simulation->gridColCount = atLeast(1, ceil(simulation->boxWidth / maxCellSide));
	simulation->gridRowCount = atLeast(1, ceil(simulation->boxHeight / maxCellSide));
	simulation->gridCellWidth = simulation->boxWidth / simulation->gridColCount;
	simulation->gridCellHeight = simulation->boxHeight / simulation->gridRowCount;
	u64 cellCount = simulation->gridColCount * simulation->gridRowCount;
	simulation->particleGrid = (Particle**) realloc(simulation->particleGrid, cellCount * sizeof(Particle*));

	assert(simulation->particleCount < (simulation->gridColCount * simulation->gridRowCount));
}

void
initSimulation(Simulation* simulation)
{
	// defaults
	
	simulation->dt = 0.005;
	simulation->separation = 2;
	simulation->bondEnergy = 50;
	simulation->wallStrength = 100;
	simulation->draggingStrength = 10;
	simulation->cutoffFactor = 2;

	// thermostat

	simulation->temperature = 10;
	simulation->viscosity = 0.1;

	// box

	f64 boxSide = 100;
	simulation->boxWidth = boxSide;
	simulation->boxHeight = boxSide;

	// init

	updateGrid(simulation);
}


void
defaultParticles(Simulation* simulation)
{
    
    setParticleCount(simulation, hexagonNumber(13));
    
    //setParticleCount(simulation, 1000);
    for (int i = 0; i < simulation->particleCount; ++i) {
        Particle* particle = simulation->particles + i;
        particle->position = simulation->separation * hexagonLatticePosition(i);
        particle->position += 0.05 * v2(randomGaussian(), randomGaussian());
        particle->velocity = v2(0, 0);
        particle->acceleration = v2(0, 0);
        Color4 orange = c4(0.8, 0.3, 0, 1);
        particle->color = orange;
    }
    
    
    printf("Initialized simulation with %d particles.", simulation->particleCount);
}

void defaultWalls(Simulation* simulation)
{
    f32 halfWidth = simulation->boxWidth / 2;
    f32 halfHeight = simulation->boxHeight / 2;
    V2 corners[] = {
        v2(-halfWidth, -halfHeight),
        v2(halfWidth, -halfHeight),
        v2(halfWidth, halfHeight),
        v2(-halfWidth, halfHeight),
    };
    
    simulation->wallCount = 4;
    simulation->walls = allocArray(Wall, simulation->wallCount);
    for (int wallIndex = 0; wallIndex < simulation->wallCount; ++wallIndex)
    {
        Wall* wall = simulation->walls + wallIndex;
        wall->start = corners[wallIndex];
        wall->end = corners[(wallIndex + 1) % simulation->wallCount];
    }
}

void
evaporationSetup(Simulation* simulation)
{
    f32 halfWidth = simulation->boxWidth / 2 / 2;
    f32 halfHeight = simulation->boxHeight / 2 / 2;
    V2 corners[] = {
        v2(-halfWidth, 0),
        v2(-halfWidth, -halfHeight),
        v2(halfWidth, -halfHeight),
        v2(halfWidth, 0),
    };
    
    simulation->wallCount = 3;
    simulation->walls = allocArray(Wall, simulation->wallCount);
    for (int wallIndex = 0; wallIndex < simulation->wallCount; ++wallIndex)
    {
        Wall* wall = simulation->walls + wallIndex;
        wall->start = corners[wallIndex];
        wall->end = corners[wallIndex + 1];
    }
    
    simulation->gravityStrength = 1;
    simulation->temperature = 20;
}

Particle*
addParticle(Simulation* simulation)
{
    setParticleCount(simulation, simulation->particleCount + 1);
    Particle* particle = simulation->particles + simulation->particleCount - 1;
    return particle;
}

void
removeParticle(Simulation* simulation, int particleIndex)
{
    Particle* particle = simulation->particles + particleIndex;
    int movedParticlesCount = simulation->particleCount - particleIndex - 1;
    memmove(particle, particle + 1, movedParticlesCount * sizeof(Particle*));
    setParticleCount(simulation, simulation->particleCount - 1);
}

V2
shortestVectorFromLine(V2 point, V2 lineStart, V2 lineEnd)
{
    V2 lineVector = lineEnd - lineStart;
	V2 pointFromLineStart = point - lineStart;
    f32 t = inner(pointFromLineStart, lineVector) / square(lineVector);
    
	V2 pointFromLine;
	if (t <= 0)
    {
        pointFromLine = pointFromLineStart;
    }
    else if (t >= 1)
    {
        pointFromLine = pointFromLineStart - lineVector;
    }
    else
    {
        pointFromLine = pointFromLineStart - t * lineVector;
    }
    return pointFromLine;
}

bool
isOverlapping(Simulation* simulation, Particle* particle)
{
    for (int particleIndex = 0; particleIndex < simulation->particleCount; ++particleIndex) {
        Particle* otherParticle = simulation->particles + particleIndex;
        if (particle == otherParticle) continue;
        V2 relativePosition = particle->position - otherParticle->position;
        f32 squaredDistance = square(relativePosition);
        f32 squaredLimit = square(particle->radius + otherParticle->radius);
        if (squaredDistance < squaredLimit)
        {
            return true;
        }
    }
    
    f32 squaredRadius = square(particle->radius);
    for (int wallIndex = 0; wallIndex < simulation->wallCount; ++wallIndex) {
        Wall* wall = simulation->walls + wallIndex;
        V2 particleFromWall = shortestVectorFromLine(particle->position, wall->start, wall->end);
        if (square(particleFromWall) < squaredRadius) {
            return true;
        }
    }
    
    return false;
}

void
applyLangevinNoise(Particle* particle, f32 temperature, f32 viscosityFactor, f32 gaussianFactor)
{
	f32 thermalVelocity = sqrt(temperature / particle->mass);

	V2 gaussianVector = v2(randomGaussian(), randomGaussian());
	particle->velocity *= viscosityFactor;
	particle->velocity += thermalVelocity * gaussianFactor * gaussianVector;
}

void
advanceSimulation(Simulation* simulation, f64 timeToSimulate)
{
	simulation->timeLeftToSimulate += timeToSimulate;

    f64 dt = simulation->dt;

    f32 viscosityFactor = exp(-0.5 * simulation->viscosity * dt);
    f32 gaussianFactor = sqrt(1 - square(viscosityFactor));

    while (simulation->timeLeftToSimulate > dt) {
        simulation->timeLeftToSimulate -= dt;

        // reset particleGrid
        int cellCount = simulation->gridRowCount * simulation->gridColCount;
        memset(simulation->particleGrid, 0, cellCount * sizeof(Particle*));

        Particle* particles = simulation->particles;

        for (int particleIndex = 0;
             particleIndex < simulation->particleCount;
             ++particleIndex)
        {
        	Particle* particle = particles + particleIndex;

        	applyLangevinNoise(particle, simulation->temperature, viscosityFactor, gaussianFactor);
        	particle->velocity += 0.5 * dt * particle->acceleration;
        	particle->position += particle->velocity * dt;
            particle->position = periodize(particle->position, simulation->boxWidth, simulation->boxHeight);

            particle->acceleration = v2(0, -simulation->gravityStrength);

            // ! Put particles in grid
            
            
            V2 normalizedPosition = v2(particle->position.x / simulation->boxWidth, particle->position.y / simulation->boxHeight) + v2(0.5, 0.5);
            int col = floor(normalizedPosition.x * simulation->gridColCount);
            int row = floor(normalizedPosition.y * simulation->gridRowCount);
            // TODO: v-- these might be redundant
            col = mod(col, simulation->gridColCount);
            row = mod(row, simulation->gridRowCount);
            int cellIndex = row * simulation->gridColCount + col;
            assert(cellIndex < cellCount);

            particle->gridCol = col;
            particle->gridRow = row;

            simulation->particleGrid[cellIndex] = particle;
        }


        // ! calculate forces

        for (int particleIndex = 0;
             particleIndex < simulation->particleCount;
             ++particleIndex)
        {
        	Particle* particle = particles + particleIndex;

        	// ! user interaction

        	if (simulation->isDragging && (particleIndex == simulation->draggedParticleIndex))
        	{
        		V2 relativePosition = simulation->mousePosition - particle->position;
        		particle->acceleration += simulation->draggingStrength / particle->mass * relativePosition;
        		particle->acceleration -= particle->velocity / particle->mass; // some friction
        	}

			// ! particle-wall interactions	
			for (int wallIndex = 0; wallIndex < simulation->wallCount; wallIndex++)
			{
				Wall* wall = simulation->walls + wallIndex;
				
                // TODO: check minus sign
                V2 particleFromWall = shortestVectorFromLine(particle->position, wall->start, wall->end);
				f32 squaredDistance = square(particleFromWall);

				if (squaredDistance < square(particle->radius))
				{
					f32 distance = sqrtf(squaredDistance);
					V2 normal = particleFromWall / distance;
					f32 overlap = particle->radius - distance;

					particle->position += overlap * normal;

					particle->velocity -= 2 * inner(particle->velocity, normal) * normal;
				}
			}

        	// ! particle-particle interactions
        	
        	f64 range = simulation->cutoffFactor * simulation->separation;
        	// TODO: maybe optimize this to be a circle? (probably not worth it)
        	int gridRadius = range / min(simulation->gridCellWidth, simulation->gridCellHeight);

        	for (int y = -gridRadius; y <= gridRadius; ++y)
        	{
        		int row = mod(particle->gridRow + y, simulation->gridRowCount);
        		int rowIndex = row * simulation->gridColCount;
        		for (int x = -gridRadius; x <= gridRadius; ++x)
        		{
        			int col = mod(particle->gridCol + x, simulation->gridColCount);
        			int cellIndex = rowIndex + col;
        			Particle* otherParticle = simulation->particleGrid[cellIndex];
        			if (otherParticle && (otherParticle < particle))
        			{
						f64 separation = simulation->separation;

						V2 relativePosition = otherParticle->position - particle->position;
				        relativePosition = periodize(relativePosition, simulation->boxWidth, simulation->boxHeight);
						f64 quadrance = square(relativePosition);

						f64 invQuadrance = 1 / quadrance;
				        f64 rInv2 = square(separation) * invQuadrance;
						f64 rInv6 = rInv2 * rInv2 * rInv2;
						f64 rInv12 = square(rInv6);
						f64 potentialEnergy = simulation->bondEnergy * (rInv12 - 2 * rInv6);
						f64 virial = simulation->bondEnergy * 12 * (rInv6 - rInv12);
						f64 forceFactor = virial * invQuadrance;
							
						particle->acceleration += forceFactor / particle->mass * relativePosition;
						otherParticle->acceleration -= forceFactor / otherParticle->mass * relativePosition;

						f64 halfPotentialEnergy = potentialEnergy / 2;
						particle->potentialEnergy = halfPotentialEnergy;
						otherParticle->potentialEnergy = halfPotentialEnergy;
        			}
        		}
        	}


        }

        for (int particleIndex = 0;
             particleIndex < simulation->particleCount;
             ++particleIndex)
        {
        	Particle* particle = particles + particleIndex;
        	particle->velocity += 0.5 * dt * particle->acceleration;
        	applyLangevinNoise(particle, simulation->temperature, viscosityFactor, gaussianFactor);

			particle->kineticEnergy = 0.5 * particle->mass * square(particle->velocity);
        }

    }
}


#endif
