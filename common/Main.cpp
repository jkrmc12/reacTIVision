/*  reacTIVision tangible interaction framework
    Main.cpp
    Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h>
#ifdef __APPLE__
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif
#ifdef LINUX
#include <signal.h>
#endif

#include <Main.h>
#include "SDLinterface.h"
#include "VisionEngine.h"

#include "FrameEqualizer.h"
#include "FrameThresholder.h"
#include "FidtrackFinder.h"
#include "CalibrationEngine.h"

VisionEngine *engine;
using namespace tinyxml2;

static void terminate (int param)
{
	if (engine!=NULL) engine->stop();
}

void printUsage(const char* app_name) {
	std::cout << "usage: " << app_name << " -c [config_file]" << std::endl;
	std::cout << "the default configuration file is " << app_name << ".xml" << std::endl;
	std::cout << "\t -n starts " << app_name << " without GUI" << std::endl;
	std::cout << "\t -l lists all available cameras" << std::endl;
	std::cout << "\t -h shows this help message" << std::endl;
	std::cout << std::endl;
}

void readSettings(application_settings *config) {

	config->port = 3333;
	sprintf(config->host,"localhost");
	sprintf(config->tree_config,"none");
	sprintf(config->grid_config,"none");
	sprintf(config->camera_config,"none");
	config->invert_x = false;
	config->invert_y = false;
	config->invert_a = false;
	config->amoeba = true;
	config->background = false;
	config->fullscreen = false;
	config->headless = false;
	config->finger_size = 0;
	config->finger_sensitivity = 100;
	config->gradient_gate = 32;
	config->tile_size = 10;
	config->thread_count = 1;
	config->display_mode = 2;

	if (strcmp( config->file, "none" ) == 0) {
#ifdef __APPLE__
	char app_path[1024];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
	CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
	CFStringGetCString( cfStringRef, app_path, 1024, kCFStringEncodingASCII);	
	CFRelease( mainBundleURL);
	CFRelease( cfStringRef);
	sprintf(config->file,"%s/Contents/Resources/reacTIVision.xml",app_path);
#elif !defined WIN32
        if (access ("./reacTIVision.xml", F_OK )==0) sprintf(config->file,"./reacTIVision.xml");
        else if (access ("/usr/share/reacTIVision/reacTIVision.xml", F_OK )==0) sprintf(config->file,"/usr/share/reacTIVision/reacTIVision.xml");
        else if (access ("/usr/local/share/reacTIVision/reacTIVision.xml", F_OK )==0) sprintf(config->file,"/usr/local/share/reacTIVision/reacTIVision.xml");
        else if (access ("/opt/share/reacTIVision/reacTIVision.xml", F_OK )==0) sprintf(config->file,"/opt/share/reacTIVision/reacTIVision.xml");
#else
        sprintf(config->file,"./reacTIVision.xml");
#endif
	}

    tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(config->file);
	if( xml_settings.Error() )
	{
		std::cout << "Error loading configuration file: " << config->file << std::endl;
		return;
	}

	XMLHandle docHandle( &xml_settings );
	XMLHandle config_root = docHandle.FirstChildElement("reactivision");

	XMLElement* tuio_element = config_root.FirstChildElement("tuio").ToElement();
	if( tuio_element!=NULL )
	{
		if(tuio_element->Attribute("host")!=NULL) sprintf(config->host,"%s",tuio_element->Attribute("host"));
		if(tuio_element->Attribute("port")!=NULL) config->port = atoi(tuio_element->Attribute("port"));
	}

	XMLElement* camera_element = config_root.FirstChildElement("camera").ToElement();
	if( camera_element!=NULL )
	{
		if(camera_element->Attribute("config")!=NULL) sprintf(config->camera_config,"%s",camera_element->Attribute("config"));
	}

	XMLElement* finger_element = config_root.FirstChildElement("finger").ToElement();
	if( finger_element!=NULL )
	{
		if(finger_element->Attribute("size")!=NULL) config->finger_size = atoi(finger_element->Attribute("size"));
		if(finger_element->Attribute("sensitivity")!=NULL) config->finger_sensitivity = atoi(finger_element->Attribute("sensitivity"));
	}

	XMLElement* image_element = config_root.FirstChildElement("image").ToElement();
	if( image_element!=NULL )
	{
		if(image_element->Attribute("display")!=NULL)  {
			if ( strcmp( image_element->Attribute("display"), "none" ) == 0 ) config->display_mode = 0;
			else if ( strcmp( image_element->Attribute("display"), "src" ) == 0 )  config->display_mode = 1;
			else if ( strcmp( image_element->Attribute("display"), "dest" ) == 0 )  config->display_mode = 2;
		}
        
		if(image_element->Attribute("equalize")!=NULL) {
			if ((strcmp( image_element->Attribute("equalize"), "true" ) == 0) ||  atoi(image_element->Attribute("equalize"))==1) config->background = true;
		}

        if(image_element->Attribute("fullscreen")!=NULL) {
            if ((strcmp( image_element->Attribute("fullscreen"), "true" ) == 0) ||  atoi(image_element->Attribute("fullscreen"))==1) config->fullscreen = true;
        }

	}

    XMLElement* threshold_element = config_root.FirstChildElement("threshold").ToElement();
    if( threshold_element!=NULL )
    {
        if(threshold_element->Attribute("gradient")!=NULL) {
            if (strcmp(threshold_element->Attribute("gradient"), "max" ) == 0) config->gradient_gate=64;
            else if (strcmp(threshold_element->Attribute("gradient"), "min" ) == 0) config->gradient_gate=0;
            else config->gradient_gate = atoi(threshold_element->Attribute("gradient"));
        }

        if(threshold_element->Attribute("tile")!=NULL) {
            if (strcmp(threshold_element->Attribute("tile"), "max" ) == 0) config->tile_size=INT_MAX;
            else if (strcmp(threshold_element->Attribute("tile"), "min" ) == 0) config->tile_size=2;
            else  config->tile_size = atoi(threshold_element->Attribute("tile"));
        }

        if(threshold_element->Attribute("threads")!=NULL) {
            if (strcmp(threshold_element->Attribute("threads"), "max" ) == 0) config->thread_count=SDL_GetCPUCount();
            else if (strcmp(threshold_element->Attribute("threads"), "min" ) == 0) config->thread_count=1;
            else {
                config->thread_count = atoi(threshold_element->Attribute("threads"));
                if(config->thread_count<1) config->thread_count = 1;
                if(config->thread_count>SDL_GetCPUCount()) config->thread_count =  SDL_GetCPUCount();                
            }
        }
        
        
 
    }

	XMLElement* fiducial_element = config_root.FirstChildElement("fiducial").ToElement();
	if( fiducial_element!=NULL )
	{
		if(fiducial_element->Attribute("engine")!=NULL)  {
			if ( strcmp( fiducial_element->Attribute("engine"), "amoeba" ) == 0 ) config->amoeba = true;
		}
		if(fiducial_element->Attribute("tree")!=NULL) sprintf(config->tree_config,"%s",fiducial_element->Attribute("tree"));
	}

	XMLElement* calibration_element = config_root.FirstChildElement("calibration").ToElement();
	if( calibration_element!=NULL )
	{
		if(calibration_element->Attribute("invert")!=NULL)  {
			if (strstr(calibration_element->Attribute("invert"),"x")>0) config->invert_x = true;
			if (strstr(calibration_element->Attribute("invert"),"y")>0) config->invert_y = true; 
			if (strstr(calibration_element->Attribute("invert"),"a")>0) config->invert_a = true; 			
		}
		if(calibration_element->Attribute("grid")!=NULL) sprintf(config->grid_config,"%s",calibration_element->Attribute("grid"));
	}

}


void writeSettings(application_settings *config) {

	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(config->file);
	if( xml_settings.Error() )
	{
		std::cout << "Error loading configuration file: " << config->file << std::endl;
		return;
	}

	char config_value[64];

	XMLHandle docHandle( &xml_settings );
	XMLHandle config_root = docHandle.FirstChildElement("reactivision");

	XMLElement* tuio_element = config_root.FirstChildElement("tuio").ToElement();
	if( tuio_element!=NULL )
	{
		if(tuio_element->Attribute("host")!=NULL) tuio_element->SetAttribute("host",config->host);
		if(tuio_element->Attribute("port")!=NULL) {
			sprintf(config_value,"%d",config->port);
			tuio_element->SetAttribute("port",config_value);
		}
	}

	XMLElement* camera_element = config_root.FirstChildElement("camera").ToElement();
	if( camera_element!=NULL )
	{
		if(camera_element->Attribute("config")!=NULL) camera_element->SetAttribute("config",config->camera_config);
	}

	XMLElement* finger_element = config_root.FirstChildElement("finger").ToElement();
	if( finger_element!=NULL )
	{
		if(finger_element->Attribute("size")!=NULL) {
			sprintf(config_value,"%d",config->finger_size);
			finger_element->SetAttribute("size",config_value);
		}
		if(finger_element->Attribute("sensitivity")!=NULL) {
			sprintf(config_value,"%d",config->finger_sensitivity);
			finger_element->SetAttribute("sensitivity",config_value);
		}
	}

	XMLElement* image_element = config_root.FirstChildElement("image").ToElement();
	if( image_element!=NULL )
	{
		if(image_element->Attribute("display")!=NULL)  {
			if (config->display_mode == 0) image_element->SetAttribute("display","none");
			else if (config->display_mode == 1) image_element->SetAttribute("display","src");
			else if (config->display_mode == 2) image_element->SetAttribute("display","dest");  
		}
		if(image_element->Attribute("equalize")!=NULL) {
			if (config->background) image_element->SetAttribute("equalize","true");
			else image_element->SetAttribute("equalize","false");
		}
        if(image_element->Attribute("fullscreen")!=NULL) {
            if (config->fullscreen) image_element->SetAttribute("fullscreen","true");
            else image_element->SetAttribute("fullscreen","false");
        }

	}

    XMLElement* threshold_element = config_root.FirstChildElement("threshold").ToElement();
    if( threshold_element!=NULL )
    {
        if(threshold_element->Attribute("gradient")!=NULL) {
            sprintf(config_value,"%d",config->gradient_gate);
            threshold_element->SetAttribute("gradient",config_value);
        }
        if(threshold_element->Attribute("tile")!=NULL) {
            sprintf(config_value,"%d",config->tile_size);
            threshold_element->SetAttribute("tile",config_value);
        }
    }


	XMLElement* fiducial_element = config_root.FirstChildElement("fiducial").ToElement();
	if( fiducial_element!=NULL )
	{
		if(fiducial_element->Attribute("engine")!=NULL)  {
			if (config->amoeba) fiducial_element->SetAttribute("engine", "amoeba"); 
		}
		if(fiducial_element->Attribute("tree")!=NULL) fiducial_element->SetAttribute("tree",config->tree_config);
	}

	XMLElement* calibration_element = config_root.FirstChildElement("calibration").ToElement();
	if( calibration_element!=NULL )
	{
		sprintf(config_value," ");
		if(calibration_element->Attribute("invert")!=NULL)  {
			if (config->invert_x) strcat(config_value,"x");
			if (config->invert_y) strcat(config_value,"y");
			if (config->invert_a) strcat(config_value,"a");
			calibration_element->SetAttribute("invert",config_value);
		}
		if(calibration_element->Attribute("grid")!=NULL) calibration_element->SetAttribute("grid",config->grid_config);
	}

	xml_settings.SaveFile(config->file);
	if( xml_settings.Error() ) std::cout << "Error saving configuration file: "  << config->file << std::endl;

}

int main(int argc, char* argv[]) {

	application_settings config;
	sprintf(config.file,"none");

	const char *app_name = "reacTIVision";
	const char *version_no = "1.6";

	bool headless = false;

	std::cout << app_name << " " << version_no << " (" << __DATE__ << ")" << std::endl << std::endl;

	if (argc>1) {
		if (strcmp( argv[1], "-h" ) == 0 ) {
			printUsage(app_name);
			return 0;
		} else if( strcmp( argv[1], "-c" ) == 0 ) {
			if (argc==3) sprintf(config.file,"%s",argv[2]);
			else {
				printUsage(app_name);
				return 0;
			}
        } else if( strcmp( argv[1], "-n" ) == 0 ) {
            headless = true;
        } else if( strcmp( argv[1], "-l" ) == 0 ) {
            CameraTool::listDevices();
            return 0;
        } else if ( (std::string(argv[1]).find("-NSDocumentRevisionsDebugMode")==0 ) || (std::string(argv[1]).find("-psn_")==0) ){
            // ignore mac specific arguments
        } else {
 			printUsage(app_name);
		}
	}

#ifndef WIN32
	signal(SIGINT,terminate);
	signal(SIGHUP,terminate);
	signal(SIGQUIT,terminate);
	signal(SIGTERM,terminate);
#endif

	readSettings(&config);
    config.headless = headless;

    UserInterface *uiface;
	engine = new VisionEngine(app_name,config.camera_config);

    if (!headless) {
        uiface = new SDLinterface(app_name,config.fullscreen);
        switch (config.display_mode) {
            case 0: uiface->setDisplayMode(uiface->NO_DISPLAY); break;
            case 1: uiface->setDisplayMode(uiface->SOURCE_DISPLAY); break;
            case 2: uiface->setDisplayMode(uiface->DEST_DISPLAY); break;
        }
        engine->setInterface(uiface);
    }

	TuioServer     *server		= NULL;
	FrameProcessor *fiducialfinder	= NULL;
	FrameProcessor *thresholder	= NULL;
	FrameProcessor *equalizer	= NULL;
	FrameProcessor *calibrator	= NULL;

	server = new TuioServer(config.host,config.port);
	server->setInversion(config.invert_x, config.invert_y, config.invert_a);

    equalizer = new FrameEqualizer();
    engine->addFrameProcessor(equalizer);
    if (config.background) equalizer->toggleFlag(' ',false);

    thresholder = new FrameThresholder(config.gradient_gate, config.tile_size, config.thread_count);
    engine->addFrameProcessor(thresholder);

    if (config.amoeba) fiducialfinder = new FidtrackFinder(server, config.tree_config, config.grid_config, config.finger_size, config.finger_sensitivity);
	engine->addFrameProcessor(fiducialfinder);

	calibrator = new CalibrationEngine(config.grid_config);
	engine->addFrameProcessor(calibrator);

	engine->start();

    if (!headless) {
        config.display_mode = uiface->getDisplayMode();
        delete uiface;
    }
    
	engine->removeFrameProcessor(calibrator);
	delete calibrator;

	if (config.amoeba) {
		config.finger_size = ((FidtrackFinder*)fiducialfinder)->getFingerSize();
		config.finger_sensitivity = ((FidtrackFinder*)fiducialfinder)->getFingerSensitivity();
	}

	engine->removeFrameProcessor(fiducialfinder);

    config.gradient_gate = ((FrameThresholder*)thresholder)->getGradientGate();
    config.tile_size = ((FrameThresholder*)thresholder)->getTileSize();
    engine->removeFrameProcessor(thresholder);
    delete thresholder;

    config.background = ((FrameEqualizer*)equalizer)->getState();
    engine->removeFrameProcessor(equalizer);
    delete equalizer;

	config.invert_x = server->getInvertX();
	config.invert_y = server->getInvertY();
	config.invert_a = server->getInvertA();

	delete fiducialfinder;
	delete engine;
	delete server;

	writeSettings(&config);
	return 0;
}
