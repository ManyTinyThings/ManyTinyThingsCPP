

#include <stdio.h>
#include <stdlib.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include "GL/glew.h"
#endif // __EMSCRIPTEN__


#include "SDL2/SDL.h"
#undef main
#include "SDL2/SDL_opengl.h"


#if DEBUG
#define assert(condition) if (!(condition)) { *(int *)0 = 0; }
#else
#define assert(condition)
#endif

#include <math.h>

#include "particle_simulation.h"

#define multilineString(src) #src

int windowPixelWidth = 1000;
int windowPixelHeight = 1000;

struct Renderer {
    GLuint buffer;
    GLuint programObject;

    GLuint translateUniform;
    GLuint scaleUniform;

    GLuint positionAttribute;
    GLuint colorAttribute;
};

struct LoopData
{
    bool isInitialized;
    bool wantsToQuit;
    f64 timestamp;
    Renderer renderer;
    Simulation simulation;
    
    bool isCKeyDown;
};

GLuint
loadShader(GLenum type, const char* shaderSrc)
{
	GLuint shader = glCreateShader(type);
    if (shader == 0)
        return 0;

    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);

	GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            printf("Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


int
initRenderer(Renderer* renderer)
{
    const char* vertexShaderSource =
    multilineString(
         
         attribute vec2 position;
         attribute vec4 color;
         
         uniform vec2 scale;
         uniform vec2 translate;
         
         varying vec4 Color;
         
         void main()
         {
             Color = color;
             vec2 finalPosition = (translate + position) * scale;
             gl_Position = vec4(finalPosition, finalPosition.y, 1.0);
         }
    );
    
    
    const char* fragmentShaderSource =
    multilineString(
         varying vec4 Color;
         
         void main() {
             gl_FragColor = Color;
         }
    );


    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint programObject = glCreateProgram();
    if (programObject == 0)
        return 0;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);


    GLint isLinked;
    glLinkProgram(programObject);
    glGetProgramiv(programObject, GL_LINK_STATUS, &isLinked);
    if (!isLinked) {
        GLint infoLen = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            printf("Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(programObject);
        return GL_FALSE;
    }


    renderer->programObject = programObject;

    glUseProgram(programObject);
    glGenBuffers(1, &renderer->buffer);

    renderer->positionAttribute = glGetAttribLocation(programObject, "position");
    renderer->colorAttribute = glGetAttribLocation(programObject, "color");

    renderer->translateUniform = glGetUniformLocation(programObject, "translate");
    renderer->scaleUniform = glGetUniformLocation(programObject, "scale");

    return GL_TRUE;
}

struct VertexColor {
    V2 vertex;
    Color4 color;
};


f64
getTime()
{
    static u64 frequency = 0;
    if (!frequency) {
        frequency = SDL_GetPerformanceFrequency();
    }
    return ((f64)SDL_GetPerformanceCounter() / frequency);
}

V2
worldFromPixel(Simulation* simulation, int x, int y)
{
    V2 result;
    result.x = ((x / ((f64) windowPixelWidth)) - 0.5) * simulation->boxWidth;
    result.y = -((y / ((f64) windowPixelHeight)) - 0.5) * simulation->boxHeight;
    return result;
}


void
loop(void* argument)
{
    LoopData* loopData = (LoopData*)argument;
    Renderer* renderer = &loopData->renderer;
    Simulation* simulation = &loopData->simulation;


    if (!loopData->isInitialized)
    {
        loopData->isInitialized = true;

        initRenderer(renderer);
        initSimulation(simulation);
        evaporationSetup(simulation);
    }

    // ! Timekeeping

    f64 previousTime = loopData->timestamp;
    loopData->timestamp = getTime();
    f64 elapsedSeconds = loopData->timestamp - previousTime;
    f64 maxFrameTime = 1.0 / 60.0;
    elapsedSeconds = atMost(maxFrameTime, elapsedSeconds);
    f64 simulatedTimePerSecond = 5;
    f64 elapsedSimulationTime = elapsedSeconds * simulatedTimePerSecond;
    
    advanceSimulation(simulation, elapsedSimulationTime);

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {
        // User requests quit
        if (event.type == SDL_QUIT)
        {
            loopData->wantsToQuit = true;
        }

        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            
            V2 mousePosition = worldFromPixel(simulation, event.button.x, event.button.y);
            int pickedParticleIndex = pickParticle(simulation, mousePosition);
            
            if (pickedParticleIndex >= 0)
            {
                simulation->isDragging = true;
                simulation->draggedParticleIndex = pickedParticleIndex;    
                simulation->mousePosition = mousePosition;
            }
        }

        if (event.type == SDL_MOUSEBUTTONUP)
        {
            if (simulation->isDragging)
            {
                simulation->isDragging = false;
            }
        }

        if (event.type == SDL_MOUSEMOTION)
        {
            simulation->mousePosition = worldFromPixel(simulation, event.motion.x, event.motion.y);
            int pickedParticleIndex = pickParticle(simulation, simulation->mousePosition);
            if (loopData->isCKeyDown && (pickedParticleIndex < 0)) {
                Particle* particle = addParticle(simulation);
                particle->position = simulation->mousePosition;
                if (isOverlapping(simulation, particle)) {
                    removeParticle(simulation, simulation->particleCount - 1);
                }
            }
        }

		if (event.type == SDL_KEYDOWN)
        {
            SDL_Scancode scancode = event.key.keysym.scancode;
			if (scancode == SDL_SCANCODE_R)
			{
				initSimulation(simulation);
			}
            else if (scancode == SDL_SCANCODE_C)
            {
                loopData->isCKeyDown = true;
            }
		}
        
        if (event.type == SDL_KEYUP)
        {
            SDL_Scancode scancode = event.key.keysym.scancode;
            if (scancode == SDL_SCANCODE_C) {
                loopData->isCKeyDown = false;
            }
        }
    }

    // ! drawing



    glViewport(0, 0, windowPixelWidth, windowPixelHeight);
    glClearColor(1, 1, 1, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUniform2f(renderer->scaleUniform, 2.0f / simulation->boxWidth, 2.0f / simulation->boxHeight);
	glUniform2f(renderer->translateUniform, 0, 0);


    glBindBuffer(GL_ARRAY_BUFFER, renderer->buffer);
    

    int bufferStride = 6;

    GLsizei vertexStride    = bufferStride * sizeof(f32);
    GLintptr positionOffset = 0 * sizeof(f32);
    GLintptr colorOffset    = 2 * sizeof(f32);

    GLuint positionAttribute = renderer->positionAttribute;
    glEnableVertexAttribArray(positionAttribute);
    glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, vertexStride, (GLvoid*)positionOffset);
    
    GLuint colorAttribute = renderer->colorAttribute;
    glEnableVertexAttribArray(colorAttribute);
    glVertexAttribPointer(colorAttribute, 4, GL_FLOAT, GL_FALSE, vertexStride, (GLvoid*)colorOffset);

    // draw particles

    {
        int discVertexCount = 20;
        V2* discVertices = (V2*) malloc(discVertexCount * sizeof(V2));

        f64 angle = tau / discVertexCount;
        f64 c = cos(angle);
        f64 s = sin(angle);

        V2 spoke = v2(1, 0);
        for (int vertexIndex = 0; vertexIndex < discVertexCount; vertexIndex++)
        {
            discVertices[vertexIndex] = spoke;

            // rotate spoke
            f64 x = spoke.x;
            f64 y = spoke.y;
            spoke.x = c * x - s * y;
            spoke.y = s * x + c * y;
        }



        int discTriangleCount = discVertexCount - 2;
        int totalVertexCount = 3 * discTriangleCount * simulation->particleCount;

        int bufferByteCount = totalVertexCount * sizeof(VertexColor);
        VertexColor* bufferData = (VertexColor*) malloc(bufferByteCount);

        VertexColor* bufferCursor = bufferData;

        for (int particleIndex = 0; particleIndex < simulation->particleCount; ++particleIndex) {

            Particle* particle = simulation->particles + particleIndex;

            V2 firstVertex = particle->position + particle->radius * discVertices[0];
            V2 secondVertex = particle->position + particle->radius * discVertices[1];

            for (int triangleIndex = 0; triangleIndex < discTriangleCount; ++triangleIndex) {
                bufferCursor->vertex = firstVertex;
                bufferCursor->color = particle->color;
                ++bufferCursor;

                bufferCursor->vertex = secondVertex;
                bufferCursor->color = particle->color;
                ++bufferCursor;

                V2 thirdVertex = particle->position + particle->radius * discVertices[triangleIndex + 2];

                bufferCursor->vertex = thirdVertex;
                bufferCursor->color = particle->color;
                ++bufferCursor;

                secondVertex = thirdVertex;
            }
        }

        glBufferData(GL_ARRAY_BUFFER, bufferByteCount, bufferData, GL_STATIC_DRAW);

            
        glDrawArrays(GL_TRIANGLES, 0, totalVertexCount);

        free(bufferData);
        free(discVertices);

    }

    
    // draw walls

    {
        int totalVertexCount = 2 * simulation->wallCount;
        int bufferByteCount = totalVertexCount * sizeof(VertexColor);

        Color4 black = c4(0, 0, 0, 1);

        VertexColor* bufferData = allocArray(VertexColor, totalVertexCount);
        VertexColor* bufferCursor = bufferData;
        for (int wallIndex = 0; wallIndex < simulation->wallCount; ++wallIndex)
        {
            Wall* wall = simulation->walls + wallIndex;

            bufferCursor->vertex = wall->start;
            bufferCursor->color = black;
            ++bufferCursor;

            bufferCursor->vertex = wall->end;
            bufferCursor->color = black;
            ++bufferCursor;
        }
        glLineWidth(3);
        glBufferData(GL_ARRAY_BUFFER, bufferByteCount, bufferData, GL_STATIC_DRAW);
        glDrawArrays(GL_LINES, 0, totalVertexCount);
		free(bufferData);
    }
}


int main(void)
{
    SDL_Window* window;
    SDL_GLContext context;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    window = SDL_CreateWindow(
        "Many Tiny Things",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        windowPixelWidth, windowPixelHeight,
        SDL_WINDOW_OPENGL);

    if (!window) {
        printf("Unable to create window: %s\n", SDL_GetError());
        return 1;
    }

    context = SDL_GL_CreateContext(window);
	
	// TODO: maybe get rid of GLEW, by using more modern GL context?

#ifndef __EMSCRIPTEN__
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK)
    {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(glewError));
    }
    glGetError();
#endif

    LoopData loopData = {};

    loopData.timestamp = getTime();

    

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(loop, &loopData, 0, 1);
#else
	while (true)
	{
		loop(&loopData);

        if (loopData.wantsToQuit)
        {
            break;
        }
		SDL_GL_SwapWindow(window);
	}
#endif

    return 0;
}