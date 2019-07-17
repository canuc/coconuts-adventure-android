/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


//BEGIN_INCLUDE(all)
#include "meow.h"
#include "texture_loading.h"
#include "wavefrontobj.h"
#include "actor.h"
#include "screen_root.h"
#include "shader_manager.h"
#include "model_manager.h"
#include "texture_manager.h"
#include "main_character.h"
#include "scene_container.h"
#include "clock.h"
#include "meow_android.h"
#include "gamethread.h"
#include "events/events.h"
#include "work_queue.h"
#include "drawable_processor.h"
#include "camera.h"
#include "timer/timer.h"
#include "platform_input.h"
#include "font_manager.h"
#include <unistd.h>
#include <string>
#include <sstream>
#include "animation/animation_manager.h"
#include "android_input.h"
#include "android_game.h"

#ifndef VARIANT
#define VARIANT "UNKNOWN"
#endif

static int initGL(EGLint w, EGLint h) {
    glViewport( 0, 0, (GLsizei) w, (GLsizei) h );
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
    //glCullFace(GL_FRONT);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0,0.0,0.0,1);
    ALOGI("GLSL Version: %s, Build V: %s",glGetString(GL_SHADING_LANGUAGE_VERSION), VARIANT);
    return 0;
}


static EGLConfig engine_select_context(EGLDisplay * display) {
    int depth;
    int r;
    int g;
    int b;
    int a;
    int s;
    int renderType;
    int surfaceType;
    int formatType;
    int numConf;
    int samples;

    eglGetConfigs(*display, NULL, 0, &numConf);

    int configurations = numConf;
    EGLConfig configToUse; //this is the one true config that we will use
    int largestNumberOfSamples = 0;

    EGLConfig* conf = new EGLConfig[numConf];
    eglGetConfigs(*display, conf, configurations, &numConf);
    LOGI("total num configs: %i", numConf);

    for(int i = 0; i < numConf; i++)
    {
        eglGetConfigAttrib(*display, conf[i], EGL_DEPTH_SIZE, &depth);
        eglGetConfigAttrib(*display, conf[i], EGL_RED_SIZE, &r);
        eglGetConfigAttrib(*display, conf[i], EGL_GREEN_SIZE, &g);
        eglGetConfigAttrib(*display, conf[i], EGL_BLUE_SIZE, &b);
        eglGetConfigAttrib(*display, conf[i], EGL_ALPHA_SIZE, &a);
        eglGetConfigAttrib(*display, conf[i], EGL_RENDERABLE_TYPE, &renderType);
        eglGetConfigAttrib(*display, conf[i], EGL_STENCIL_SIZE, &s);
        eglGetConfigAttrib(*display, conf[i], EGL_SURFACE_TYPE, &surfaceType);
        eglGetConfigAttrib(*display, conf[i], EGL_NATIVE_VISUAL_ID, &formatType);
        eglGetConfigAttrib(*display, conf[i], EGL_SAMPLES, &samples);

        LOGI("(R%i,G%i,B%i,A%i)depth:(%i) stencil:(%i) surfaceType:(%i) renderType:(%i) formatType:(%i)",r,g,b,a,depth,s,surfaceType, renderType);


        if((renderType & EGL_OPENGL_ES2_BIT) > 0 &&
           (surfaceType & EGL_WINDOW_BIT) > 0 &&
           (formatType & WINDOW_FORMAT_RGBA_8888) > 0 &&
           (r == 8) && (g == 8) && (b == 8) &&
           depth > 0 ) {
            configToUse=conf[i];
            LOGI("Config #%i" , i);
            LOGI("(R%i,G%i,B%i,A%i) %i_depth %i_stencil %i_surfaceType %i_NativeVisualId %ix",r,g,b,a,depth,s,surfaceType,formatType,samples);
        }
    }

    return configToUse;
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    //now create the OpenGL ES2 context
    const EGLint contextAttribs[] =
    {
            EGL_CONTEXT_CLIENT_VERSION , 2,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;
    EGLint majorVersion;
    EGLint minorVersion;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, &majorVersion, &minorVersion);
    LOGI("EGL %i.%i", majorVersion,minorVersion);

    config = engine_select_context(&display);
    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    //eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, contextAttribs);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->density = density_dpi(engine);

    initGL(w,h);
    return 0;
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (!engine->game) {
        return;
    }

    delete engine->game;
    engine->game = NULL;

    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }

    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (engine->drawableState) {
        AndroidInput * input = (AndroidInput *) engine->drawableState->userController;
        return input->addAndroidInput(event);
    } else {
        return false;
    }
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it     ready.
            if (engine->app->window != NULL) {
                android_fopen_set_asset_manager(engine->assetManager);
                engine_init_display(engine);
                Game * game = new AndroidGame(engine);
                engine->game = game;
                game->bootstrap();
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);

            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                        engine->accelerometerSensor, (1000L/60)*1000);
            }
            engine->animating = 1;
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            //engine_draw_frame(engine);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
            state->looper, LOOPER_ID_USER, NULL, NULL);
    populate_asset_manager(&engine);

    DrawableProcessor drawableProcessor(&engine);
    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.
    while (1) {
        int ident;
        int events;

        struct android_poll_source* source;
        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(engine.app, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                       &event, 1) > 0) {
                         if (engine.drawableState) {
                             ((AndroidInput *) engine.drawableState->userController)->handleTilt(&event);
                         }
                    }
                }
            }
        }
        // Check if we are exiting.
        if (state->destroyRequested != 0) {
            engine_term_display(&engine);
            while(!getIsShutdown()) {
                usleep(50000);
            }
            return;
        }

        // Read all pending events.
        DrawableEvent * drawableEvent = NULL;
        while (engine.drawQueue && (drawableEvent=engine.drawQueue->getEvent()) != NULL) {
            drawableProcessor.processDrawableEvent(drawableEvent);
        }

        if (engine.game) {
            engine.game->drawframe();
        }
    }
}
//END_INCLUDE(all)
