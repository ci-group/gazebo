/*
 * Copyright 2011 Nate Koenig
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
/* Desc: Middleman between OGRE and Gazebo
 * Author: Nate Koenig
 * Date: 13 Feb 2006
 */
#define BOOST_FILESYSTEM_VERSION 2

#include <boost/filesystem.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include <iostream>

#include "rendering/ogre_gazebo.h"

#include "gazebo_config.h"

#include "transport/Transport.hh"
#include "transport/Node.hh"
#include "transport/Subscriber.hh"

#include "common/Common.hh"
#include "common/Color.hh"
#include "common/Events.hh"
#include "common/Exception.hh"
#include "common/Console.hh"
#include "common/SystemPaths.hh"

#include "rendering/Material.hh"
#include "rendering/RenderEvents.hh"
#include "rendering/RTShaderSystem.hh"
#include "rendering/WindowManager.hh"
#include "rendering/Scene.hh"
#include "rendering/Grid.hh"
#include "rendering/Visual.hh"
#include "rendering/UserCamera.hh"
#include "rendering/RenderEngine.hh"

using namespace gazebo;
using namespace rendering;


//////////////////////////////////////////////////
RenderEngine::RenderEngine()
{
  this->logManager = NULL;
  this->root = NULL;

  this->dummyDisplay = NULL;

  this->initialized = false;

  this->renderPathType = NONE;
}


//////////////////////////////////////////////////
RenderEngine::~RenderEngine()
{
  // this->Fini();
}

//////////////////////////////////////////////////
void RenderEngine::Load()
{
  if (!this->CreateContext())
  {
    gzwarn << "Unable to create X window. Rendering will be disabled\n";
    return;
  }

  this->connections.push_back(event::Events::ConnectPreRender(
        boost::bind(&RenderEngine::PreRender, this)));
  this->connections.push_back(event::Events::ConnectRender(
        boost::bind(&RenderEngine::Render, this)));
  this->connections.push_back(event::Events::ConnectPostRender(
        boost::bind(&RenderEngine::PostRender, this)));

  // Create a new log manager and prevent output from going to stdout
  this->logManager = new Ogre::LogManager();

  std::string logPath = common::SystemPaths::Instance()->GetLogPath();
  logPath += "/ogre.log";

  this->logManager->createLog(logPath, true, false, false);

  if (this->root)
  {
    gzerr << "Attempting to load, but root already exist\n";
    return;
  }

  // Make the root
  try
  {
    this->root = new Ogre::Root();
  }
  catch(Ogre::Exception &e)
  {
    gzthrow("Unable to create an Ogre rendering environment, no Root ");
  }

  // Load all the plugins
  this->LoadPlugins();

  // Setup the rendering system, and create the context
  this->SetupRenderSystem();

  // Initialize the root node, and don't create a window
  this->root->initialise(false);

  // Setup the available resources
  this->SetupResources();

  std::stringstream stream;
  stream << (int32_t)this->dummyWindowId;

  WindowManager::Instance()->CreateWindow(stream.str(), 1, 1);
  this->CheckSystemCapabilities();
}

//////////////////////////////////////////////////
ScenePtr RenderEngine::CreateScene(const std::string &_name,
                                   bool _enableVisualizations)
{
  if (this->renderPathType == NONE)
    return ScenePtr();

  ScenePtr scene(new Scene(_name, _enableVisualizations));
  this->scenes.push_back(scene);

  scene->Load();
  if (this->initialized)
    scene->Init();
  else
    gzerr << "RenderEngine is not initialized\n";

  rendering::Events::createScene(_name);

  return scene;
}

//////////////////////////////////////////////////
void RenderEngine::RemoveScene(const std::string &_name)
{
  if (this->renderPathType == NONE)
    return;

  std::vector<ScenePtr>::iterator iter;

  for (iter = this->scenes.begin(); iter != this->scenes.end(); ++iter)
    if ((*iter)->GetName() == _name)
      break;

  if (iter != this->scenes.end())
  {
    RTShaderSystem::Instance()->Clear();
    rendering::Events::removeScene(_name);

    (*iter)->Clear();
    (*iter).reset();
    this->scenes.erase(iter);
  }
}

//////////////////////////////////////////////////
ScenePtr RenderEngine::GetScene(const std::string &_name)
{
  if (this->renderPathType == NONE)
    return ScenePtr();

  std::vector<ScenePtr>::iterator iter;

  for (iter = this->scenes.begin(); iter != this->scenes.end(); ++iter)
    if ((*iter)->GetName() == _name)
      return (*iter);

  return ScenePtr();
}

//////////////////////////////////////////////////
ScenePtr RenderEngine::GetScene(unsigned int index)
{
  if (index < this->scenes.size())
    return this->scenes[index];
  else
  {
    gzerr << "Invalid Scene Index[" << index << "]\n";
    return ScenePtr();
  }
}

//////////////////////////////////////////////////
unsigned int RenderEngine::GetSceneCount() const
{
  return this->scenes.size();
}

//////////////////////////////////////////////////
void RenderEngine::PreRender()
{
  this->root->_fireFrameStarted();
}

//////////////////////////////////////////////////
void RenderEngine::Render()
{
}

//////////////////////////////////////////////////
void RenderEngine::PostRender()
{
  // _fireFrameRenderingQueued needs to be here for CEGUI to work
  this->root->_fireFrameRenderingQueued();
  this->root->_fireFrameEnded();
}

//////////////////////////////////////////////////
void RenderEngine::Init()
{
  if (this->renderPathType == NONE)
    return;

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init();
  this->initialized = false;

  Ogre::ColourValue ambient;

  /// Create a dummy rendering context.
  /// This will allow gazebo to run headless. And it also allows OGRE to
  /// initialize properly

  // Set default mipmap level (NB some APIs ignore this)
  Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

  // init the resources
  Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

  Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(
      Ogre::TFO_ANISOTROPIC);

  RTShaderSystem::Instance()->Init();
  rendering::Material::CreateMaterials();

  for (unsigned int i = 0; i < this->scenes.size(); i++)
    this->scenes[i]->Init();

  this->initialized = true;
}


//////////////////////////////////////////////////
void RenderEngine::Fini()
{
  if (!this->initialized)
    return;

  this->node->Fini();
  this->connections.clear();

  // TODO: this was causing a segfault on shutdown
  // Close all the windows first;
  WindowManager::Instance()->Fini();

  RTShaderSystem::Instance()->Fini();

  this->scenes.clear();

  // TODO: this was causing a segfault. Need to debug, and put back in
  if (this->root)
  {
    this->root->shutdown();
    /*const Ogre::Root::PluginInstanceList ll =
     this->root->getInstalledPlugins();

    for (Ogre::Root::PluginInstanceList::const_iterator iter = ll.begin();
         iter != ll.end(); iter++)
    {
      this->root->unloadPlugin((*iter)->getName());
      this->root->uninstallPlugin(*iter);
    }
    */

    try
    {
      delete this->root;
    }
    catch(...)
    { }
  }
  this->root = NULL;

  delete this->logManager;
  this->logManager = NULL;

  if (this->dummyDisplay)
  {
    glXDestroyContext(static_cast<Display*>(this->dummyDisplay),
                      static_cast<GLXContext>(this->dummyContext));
    XDestroyWindow(static_cast<Display*>(this->dummyDisplay),
                   this->dummyWindowId);
    XCloseDisplay(static_cast<Display*>(this->dummyDisplay));
    this->dummyDisplay = NULL;
  }

  this->initialized = false;
}

//////////////////////////////////////////////////
void RenderEngine::Save(std::string &prefix, std::ostream &stream)
{
  stream << prefix << "<rendering:ogre>\n";
  // this->scenes[0]->Save(prefix, stream);
  stream << prefix << "</rendering:ogre>\n";
}

//////////////////////////////////////////////////
void RenderEngine::LoadPlugins()
{
  std::list<std::string>::iterator iter;
  std::list<std::string> ogrePaths =
    common::SystemPaths::Instance()->GetOgrePaths();

  for (iter = ogrePaths.begin();
       iter!= ogrePaths.end(); ++iter)
  {
    std::string path(*iter);
    DIR *dir = opendir(path.c_str());

    if (dir == NULL)
    {
      continue;
    }
    closedir(dir);

    std::vector<std::string> plugins;
    std::vector<std::string>::iterator piter;

    plugins.push_back(path+"/RenderSystem_GL.so");
    plugins.push_back(path+"/Plugin_ParticleFX.so");
    plugins.push_back(path+"/Plugin_BSPSceneManager.so");
    plugins.push_back(path+"/Plugin_OctreeSceneManager.so");

    // This is needed by the Ogre::Terrain System.
    // We should spend some tim fixing Ogre::Terrain so that GLSL is
    // supported.
    plugins.push_back(path+"/Plugin_CgProgramManager.so");

    for (piter = plugins.begin(); piter!= plugins.end(); ++piter)
    {
      try
      {
        // Load the plugin into OGRE
        this->root->loadPlugin(*piter);
      }
      catch(Ogre::Exception &e)
      {
        if ((*piter).find("RenderSystem") != std::string::npos)
        {
          gzerr << "Unable to load Ogre Plugin[" << *piter
                << "]. Rendering will not be possible."
                << "Make sure you have installed OGRE and Gazebo properly.\n";
        }
        else if ((*piter).find("CgProgramManager") != std::string::npos)
        {
          gzwarn << "Unable to load Ogre Plugin[" << *piter
                 << "]Heightmaps(Terrain) will not display properly."
                 << "You'll need to install Ogre3d from source."
                 << "Please visit www.ogre3d.org.\n";
        }
      }
    }
  }
}

/////////////////////////////////////////////////
void RenderEngine::AddResourcePath(const std::string &_uri)
{
  if (_uri == "__default__" || _uri.empty())
    return;

  std::string path = common::find_file_path(_uri);

  if (path.empty())
  {
    gzerr << "URI doesn't exist[" << _uri << "]\n";
    return;
  }

  try
  {
    if (!Ogre::ResourceGroupManager::getSingleton().resourceLocationExists(
          path, "General"))
    {
      Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
          path, "FileSystem", "General", true);
      Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(
          "General");


      // Parse all material files in the path if any exist
      boost::filesystem::path dir(path);
      std::list<boost::filesystem::path> resultSet;

      if (boost::filesystem::exists(dir) &&
          boost::filesystem::is_directory(dir))
      {
        std::vector<boost::filesystem::path> paths;
        std::copy(boost::filesystem::directory_iterator(dir),
            boost::filesystem::directory_iterator(),
            std::back_inserter(paths));
        std::sort(paths.begin(), paths.end());

        // Iterate over all the models in the current gazebo path
        for (std::vector<boost::filesystem::path>::iterator dIter =
            paths.begin(); dIter != paths.end(); ++dIter)
        {
          if (dIter->filename().find(".material") != std::string::npos)
          {
            std::string fullPath = path + "/" + dIter->filename();

            Ogre::DataStreamPtr stream =
              Ogre::ResourceGroupManager::getSingleton().openResource(
                  fullPath, "General");

            // There is a material file under there somewhere, read the thing in
            try
            {
              Ogre::MaterialManager::getSingleton().parseScript(
                  stream, "General");
              Ogre::MaterialPtr matPtr =
                Ogre::MaterialManager::getSingleton().getByName(fullPath);

              if (!matPtr.isNull())
              {
                // is this necessary to do here? Someday try it without
                matPtr->compile();
                matPtr->load();
              }
            }
            catch(Ogre::Exception& e)
            {
              gzerr << "Unable to parse material file[" << fullPath << "]\n";
            }
            stream->close();
          }
        }
      }
    }
  }
  catch(Ogre::Exception)
  {
    gzthrow(std::string("Unable to load Ogre Resources.\nMake sure the") +
        "resources path in the world file is set correctly.");
  }
}

//////////////////////////////////////////////////
RenderEngine::RenderPathType RenderEngine::GetRenderPathType() const
{
  return this->renderPathType;
}

//////////////////////////////////////////////////
void RenderEngine::SetupResources()
{
  std::vector< std::pair<std::string, std::string> > archNames;
  std::vector< std::pair<std::string, std::string> >::iterator aiter;
  std::list<std::string>::const_iterator iter;
  std::list<std::string> paths =
    common::SystemPaths::Instance()->GetGazeboPaths();

  std::list<std::string> mediaDirs;
  mediaDirs.push_back("media");
  mediaDirs.push_back("Media");

  for (iter = paths.begin(); iter != paths.end(); ++iter)
  {
    DIR *dir;
    if ((dir = opendir((*iter).c_str())) == NULL)
    {
      continue;
    }
    closedir(dir);

    archNames.push_back(
        std::make_pair((*iter)+"/", "General"));

    for (std::list<std::string>::iterator mediaIter = mediaDirs.begin();
         mediaIter != mediaDirs.end(); ++mediaIter)
    {
      std::string prefix = (*iter) + "/" + (*mediaIter);

      archNames.push_back(
          std::make_pair(prefix, "General"));
      archNames.push_back(
          std::make_pair(prefix + "/skyx", "SkyX"));
      archNames.push_back(
          std::make_pair(prefix + "/rtshaderlib", "General"));
      archNames.push_back(
          std::make_pair(prefix + "/materials/programs", "General"));
      archNames.push_back(
          std::make_pair(prefix + "/materials/scripts", "General"));
      archNames.push_back(
          std::make_pair(prefix + "/materials/textures", "General"));
      archNames.push_back(
          std::make_pair(prefix + "/media/models", "General"));
      archNames.push_back(
          std::make_pair(prefix + "/fonts", "Fonts"));
      archNames.push_back(
          std::make_pair(prefix + "/gui/looknfeel", "LookNFeel"));
      archNames.push_back(
          std::make_pair(prefix + "/gui/schemes", "Schemes"));
      archNames.push_back(
          std::make_pair(prefix + "/gui/imagesets", "Imagesets"));
      archNames.push_back(
          std::make_pair(prefix + "/gui/fonts", "Fonts"));
      archNames.push_back(
          std::make_pair(prefix + "/gui/layouts", "Layouts"));
      archNames.push_back(
          std::make_pair(prefix + "/gui/animations", "Animations"));
    }

    for (aiter = archNames.begin(); aiter!= archNames.end(); ++aiter)
    {
      try
      {
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
            aiter->first, "FileSystem", aiter->second);
      }
      catch(Ogre::Exception)
      {
        gzthrow(std::string("Unable to load Ogre Resources.\n") +
            "Make sure the resources path in the world file is set correctly.");
      }
    }
  }
}

//////////////////////////////////////////////////
void RenderEngine::SetupRenderSystem()
{
  Ogre::RenderSystem *renderSys;
  const Ogre::RenderSystemList *rsList;

  // Set parameters of render system (window size, etc.)
#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
  rsList = this->root->getAvailableRenderers();
#else
  rsList = &(this->root->getAvailableRenderers());
#endif

  int c = 0;

  renderSys = NULL;

  do
  {
    if (c == static_cast<int>(rsList->size()))
      break;

    renderSys = rsList->at(c);
    c++;
  }
  while (renderSys &&
         renderSys->getName().compare("OpenGL Rendering Subsystem") != 0);

  if (renderSys == NULL)
  {
    gzthrow("unable to find OpenGL rendering system. OGRE is probably "
            "installed incorrectly. Double check the OGRE cmake output, "
            "and make sure OpenGL is enabled.");
  }

  // We operate in windowed mode
  renderSys->setConfigOption("Full Screen", "No");

  /// We used to allow the user to set the RTT mode to PBuffer, FBO, or Copy.
  ///   Copy is slow, and there doesn't seem to be a good reason to use it
  ///   PBuffer limits the size of the renderable area of the RTT to the
  ///           size of the first window created.
  ///   FBO seem to be the only good option
  renderSys->setConfigOption("RTT Preferred Mode", "FBO");

  renderSys->setConfigOption("FSAA", "4");

  this->root->setRenderSystem(renderSys);
}

/////////////////////////////////////////////////
bool RenderEngine::CreateContext()
{
  bool result = true;

  try
  {
    this->dummyDisplay = XOpenDisplay(0);
    if (!this->dummyDisplay)
    {
      gzerr << "Can't open display: " << XDisplayName(0) << "\n";
      return false;
    }

    int screen = DefaultScreen(this->dummyDisplay);

    int attribList[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16,
      GLX_STENCIL_SIZE, 8, None };

    XVisualInfo *dummyVisual = glXChooseVisual(
        static_cast<Display*>(this->dummyDisplay),
        screen, static_cast<int *>(attribList));

    if (!dummyVisual)
    {
      gzerr << "Unable to create glx visual\n";
      return false;
    }

    this->dummyWindowId = XCreateSimpleWindow(
        static_cast<Display*>(this->dummyDisplay),
        RootWindow(static_cast<Display*>(this->dummyDisplay), screen),
        0, 0, 1, 1, 0, 0, 0);

    this->dummyContext = glXCreateContext(
        static_cast<Display*>(this->dummyDisplay),
        dummyVisual, NULL, 1);

    if (!this->dummyContext)
    {
      gzerr << "Unable to create glx context\n";
      return false;
    }

    glXMakeCurrent(static_cast<Display*>(this->dummyDisplay),
        this->dummyWindowId, static_cast<GLXContext>(this->dummyContext));
  }
  catch(...)
  {
    result = false;
  }


  return result;
}

/////////////////////////////////////////////////
void RenderEngine::CheckSystemCapabilities()
{
  const Ogre::RenderSystemCapabilities *capabilities;
  Ogre::RenderSystemCapabilities::ShaderProfiles profiles;
  Ogre::RenderSystemCapabilities::ShaderProfiles::const_iterator iter;

  capabilities = this->root->getRenderSystem()->getCapabilities();
  profiles = capabilities->getSupportedShaderProfiles();

  bool hasFragmentPrograms =
    capabilities->hasCapability(Ogre::RSC_FRAGMENT_PROGRAM);

  bool hasVertexPrograms =
    capabilities->hasCapability(Ogre::RSC_VERTEX_PROGRAM);

  // bool hasGeometryPrograms =
  //  capabilities->hasCapability(Ogre::RSC_GEOMETRY_PROGRAM);

  // bool hasRenderToVertexBuffer =
  //  capabilities->hasCapability(Ogre::RSC_HWRENDER_TO_VERTEX_BUFFER);

  // int multiRenderTargetCount = capabilities->getNumMultiRenderTargets();

  bool hasFBO =
    capabilities->hasCapability(Ogre::RSC_FBO);

  bool hasGLSL =
    std::find(profiles.begin(), profiles.end(), "glsl") != profiles.end();

  if (!hasGLSL || !hasFBO)
  {
    gzerr << "Your machine does not meet the minimal rendering requirements.\n";
    if (!hasGLSL)
      gzerr << "   GLSL is missing.\n";
    if (!hasFBO)
      gzerr << "   Frame Buffer Objects (FBO) is missing.\n";
  }

  if (hasVertexPrograms && hasFragmentPrograms)
    this->renderPathType = RenderEngine::FORWARD;
  else
    this->renderPathType = RenderEngine::VERTEX;

  // Disable deferred rendering for now. Needs more work.
  // if (hasRenderToVertexBuffer && multiRenderTargetCount >= 8)
  //  this->renderPathType = RenderEngine::DEFERRED;
}
