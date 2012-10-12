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
#ifndef _SCENE_HH_
#define _SCENE_HH_

#include <vector>
#include <map>
#include <string>
#include <list>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "sdf/sdf.hh"
#include "msgs/msgs.hh"

#include "rendering/RenderTypes.hh"

#include "transport/TransportTypes.hh"
#include "common/Events.hh"
#include "common/Color.hh"
#include "math/Vector2i.hh"

namespace SkyX
{
  class SkyX;
  class BasicController;
}

namespace Ogre
{
  class SceneManager;
  class RaySceneQuery;
  class Node;
  class Entity;
  class Mesh;
  class Vector3;
  class Quaternion;
}

namespace boost
{
  class mutex;
}

namespace gazebo
{
  namespace rendering
  {
    class Projector;
    class Light;
    class Visual;
    class Grid;
    class Heightmap;

    /// \addtogroup gazebo_rendering
    /// \{

    /// \class Scene Scene.hh rendering/rendering.hh
    /// \brief Representation of an entire scene graph.
    ///
    /// Maintains all the Visuals, Lights, and Cameras for a World.
    class Scene : public boost::enable_shared_from_this<Scene>
    {
      /// \brief Constructor.
      private: Scene() {}

      /// \brief Constructor.
      /// \param[in] _name Name of the scene.
      /// \param[in] _enableVisualizations True to enable visualizations,
      /// this should be set to true for user interfaces, and false for
      /// sensor generation.
      public: Scene(const std::string &_name,
                    bool _enableVisualizations = false);

      /// \brief Destructor
      public: virtual ~Scene();

      /// \brief Load the scene from a set of parameters.
      /// \param[in] _scene SDF scene element to load.
      public: void Load(sdf::ElementPtr _scene);

      /// \brief Load the scene with default parameters.
      public: void Load();

      /// \brief Init rendering::Scene.
      public: void Init();

      /// \brief Process all received messages.
      public: void PreRender();

      /// \brief Get the OGRE scene manager.
      /// \return Pointer to the Ogre SceneManager.
      public: Ogre::SceneManager *GetManager() const;

      /// \brief Get the name of the scene.
      /// \return Name of the scene.
      public: std::string GetName() const;

      /// \brief Set the ambient color.
      /// \param[in] _color The ambient color to use.
      public: void SetAmbientColor(const common::Color &_color);

      /// \brief Get the ambient color.
      /// \return The scene's ambient color.
      public: common::Color GetAmbientColor() const;

      /// \brief Set the background color.
      /// \param[in] _color The background color.
      public: void SetBackgroundColor(const common::Color &_color);

      /// \brief Get the background color.
      /// \return The background color.
      public: common::Color GetBackgroundColor() const;

      /// \brief Create a square grid of cells.
      /// \param[in] _cellCount Number of grid cells in one direction.
      /// \param[in] _cellLength Length of one grid cell.
      /// \param[in] _lineWidth Width of the grid lines.
      /// \param[in] _color Color of the grid lines.
      public: void CreateGrid(uint32_t _cellCount, float _cellLength,
                              float _lineWidth, const common::Color &_color);

      /// \brief Get a grid based on an index. Index must be between 0 and
      /// Scene::GetGridCount.
      /// \param[in] _index Index of the grid.
      public: Grid *GetGrid(uint32_t _index) const;

      /// \brief Get the number of grids.
      /// \return The number of grids.
      public: uint32_t GetGridCount() const;

      /// \brief Create a camera
      /// \param[in] _name Name of the new camera.
      /// \param[in] _autoRender True to allow Gazebo to automatically
      /// render the camera. This should almost always be true.
      /// \return Pointer to the new camera.
      public: CameraPtr CreateCamera(const std::string &_name,
                                     bool _autoRender = true);

      /// \brief Create depth camera
      /// \param[in] _name Name of the new camera.
      /// \param[in] _autoRender True to allow Gazebo to automatically
      /// render the camera. This should almost always be true.
      /// \return Pointer to the new camera.
     public: DepthCameraPtr CreateDepthCamera(const std::string &_name,
                                               bool _autoRender = true);

      /// \brief Create laser that generates data from rendering.
      /// \param[in] _name Name of the new laser.
      /// \param[in] _autoRender True to allow Gazebo to automatically
      /// render the camera. This should almost always be true.
      /// \return Pointer to the new laser.
     public: GpuLaserPtr CreateGpuLaser(const std::string &_name,
                                         bool _autoRender = true);

      /// \brief Get the number of cameras in this scene
      /// \return Number of lasers.
      public: uint32_t GetCameraCount() const;

      /// \brief Get a camera based on an index. Index must be between
      /// 0 and Scene::GetCameraCount.
      /// \param[in] _index Index of the camera to get.
      /// \return Pointer to the camera. Or NULL if the index is invalid.
      public: CameraPtr GetCamera(uint32_t _index) const;

      /// \brief Get a camera by name.
      /// \param[in] _name Name of the camera.
      /// \return Pointer to the camera. Or NULL if the name is invalid.
      public: CameraPtr GetCamera(const std::string &_name) const;

      /// \brief Create a user camera.
      ///
      /// A user camera is one design for use with a GUI.
      /// \param[in] _name Name of the UserCamera.
      /// \return A pointer to the new UserCamera.
      public: UserCameraPtr CreateUserCamera(const std::string &_name);

      /// \brief Get the number of user cameras in this scene
      /// \return The number of user cameras.
      public: uint32_t GetUserCameraCount() const;

      /// \brief Get a user camera by index. The index value must be between
      /// 0 and Scene::GetUserCameraCount.
      /// \param[in] _index Index of the UserCamera to get.
      /// \return Pointer to the UserCamera, or NULL if the index was
      /// invalid.
      public: UserCameraPtr GetUserCamera(uint32_t _index) const;

      /// \brief Get a light by name.
      /// \param[in] _name Name of the light to get.
      /// \return Pointer to the light, or NULL if the light was not found.
      public: LightPtr GetLight(const std::string &_name) const;

      /// \brief Get the count of the lights.
      /// \return The number of lights.
      public: uint32_t GetLightCount() const;

      /// \brief Get a light based on an index. The index must be between
      /// 0 and Scene::GetLightCount.
      /// \param[in] _index Index of the light.
      /// \return Pointer to the Light or NULL if index was invalid.
      public: LightPtr GetLight(uint32_t _index) const;

      /// \brief Get a visual by name
      public: VisualPtr GetVisual(const std::string &_name) const;

      /// \brief Select a visual by name.
      /// \param[in] _name Name of the visual to select.
      public: void SelectVisual(const std::string &_name);

      /// \brief Get an entity at a pixel location using a camera. Used for
      ///        mouse picking.
      /// \param[in] camera The ogre camera, used to do mouse picking
      /// \param[in] mousePos The position of the mouse in screen coordinates
      /// \param[out] _mod Used for object manipulation
      /// \return The selected entity, or NULL
      public: VisualPtr GetVisualAt(CameraPtr _camera,
                                    const math::Vector2i &_mousePos,
                                    std::string &mod);

      /// \brief Move the visual to be ontop of the nearest visual below it.
      /// \param[in] _visualName Name of the visual to move.
      public: void SnapVisualToNearestBelow(const std::string &_visualName);

      /// \brief Get a visual at a mouse position.
      /// \param[in] _camera Pointer to the camera used to project the mouse
      /// position.
      /// \param[in] _mousePos The 2d position of the mouse in pixels.
      /// \return Pointer to the visual, NULL if none found.
      public: VisualPtr GetVisualAt(CameraPtr _camera,
                                    const math::Vector2i &_mousePos);

      /// \brief Get a model's visual at a mouse position.
      /// \param[in] _camera Pointer to the camera used to project the mouse
      /// position.
      /// \param[in] _mousePos The 2d position of the mouse in pixels.
      /// \return Pointer to the visual, NULL if none found.
      public: VisualPtr GetModelVisualAt(CameraPtr _camera,
                                         const math::Vector2i &_mousePos);


      /// \brief Get the closest visual below a given visual.
      /// \param[in] _visualName Name of the visual to search below.
      /// \return Pointer to the visual below, or NULL if no visual.
      public: VisualPtr GetVisualBelow(const std::string &_visualName);

      /// \brief Get a visual directly below a point.
      /// \param[in] _pt 3D point to get the visual below.
      /// \param[out] _visuals The visuals below the point order in
      /// proximity.
      public: void GetVisualsBelowPoint(const math::Vector3 &_pt,
                                        std::vector<VisualPtr> &_visuals);


      /// \brief Get the world pos of a the first contact at a pixel location.
      /// \param[in] _camera Pointer to the camera.
      /// \param[in] _mousePos 2D position of the mouse in pixels.
      /// \return 3D position of the first contact point.
      public: math::Vector3 GetFirstContact(CameraPtr _camera,
                                            const math::Vector2i &_mousePos);

      /// \brief Print the scene graph to std_out.
      public: void PrintSceneGraph();

      /// \brief Hide or show a visual.
      /// \param[in] _name Name of the visual to change.
      /// \param[in] _visible True to make visual visible, False to make it
      /// invisible.
      public: void SetVisible(const std::string &_name, bool _visible);

      /// \brief Draw a named line.
      /// \param[in] _start Start position of the line.
      /// \param[in] _end End position of the line.
      /// \param[in] _name Name of the line.
      public: void DrawLine(const math::Vector3 &_start,
                            const math::Vector3 &_end,
                            const std::string &_name);

      /// \brief Set the fog parameters.
      /// \param[in] _type Type of fog: "linear", "exp", or "exp2".
      /// \param[in] _color Color of the fog.
      /// \param[in] _density Fog density.
      /// \param[in] _start Distance from camera to start the fog.
      /// \param[in] _end Distance from camera at which the fog is at max
      /// density.
      public: void SetFog(const std::string &_type,
                           const common::Color &_color,
                           double _density, double _start, double _end);

      /// \brief Get the scene ID.
      /// \return The ID of the scene.
      public: uint32_t GetId() const;

      /// \brief Get the scene Id as a string.
      /// \return The ID as a string.
      public: std::string GetIdString() const;

      /// \brief Set whether shadows are on or off
      /// \param[in] _value True to enable shadows, False to disable
      public: void SetShadowsEnabled(bool _value);

      /// \brief Get whether shadows are on or off
      /// \return True if shadows are enabled.
      public: bool GetShadowsEnabled() const;

      /// \brief Add a visual to the scene
      /// \param[in] _vis Visual to add.
      public: void AddVisual(VisualPtr _vis);

      /// \brief Remove a visual from the scene.
      /// \param[in] _vis Visual to remove.
      public: void RemoveVisual(VisualPtr _vis);

      /// \brief Set the grid on or off
      /// \param[in] _enabled Set to true to turn on the grid
      public: void SetGrid(bool _enabled);

      /// \brief Get the top level world visual.
      /// \return Pointer to the world visual.
      public: VisualPtr GetWorldVisual() const;

      /// \brief Remove the name of scene from a string.
      /// \param[in] _name Name to string the scene name from.
      /// \return The stripped name.
      public: std::string StripSceneName(const std::string &_name) const;

      /// \brief Get a pointer to the heightmap.
      /// \return Pointer to the heightmap, NULL if no heightmap.
      public: Heightmap *GetHeightmap() const;

      /// \brief Clear rendering::Scene
      public: void Clear();

      /// \brief Clone a visual.
      /// \param[in] _visualName Name of the visual to clone.
      /// \param[in] _newName New name of the visual.
      /// \return Pointer to the cloned visual.
      public: VisualPtr CloneVisual(const std::string &_visualName,
                                    const std::string &_newName);

      /// \brief Helper function to setup the sky.
      private: void SetSky();

      /// \brief Initialize the deferred shading render path.
      private: void InitDeferredShading();

      /// \brief Helper function for GetVisualAt functions.
      /// \param[in] _camera Pointer to the camera.
      /// \param[in] _mousePos 2D position of the mouse in pixels.
      /// \param[in] _ignorSelectionObj True to ignore selection objects,
      /// which are GUI objects use to manipulate objects.
      /// \return Pointer to the Ogre::Entity, NULL if none.
      private: Ogre::Entity *GetOgreEntityAt(CameraPtr _camera,
                                             const math::Vector2i &_mousePos,
                                             bool _ignorSelectionObj);

      // \brief Get the mesh information for the given mesh.
      // Code found in Wiki: www.ogre3d.org/wiki/index.php/RetrieveVertexData
      private: void GetMeshInformation(const Ogre::Mesh *mesh,
                                       size_t &vertex_count,
                                       Ogre::Vector3* &vertices,
                                       size_t &index_count,
                                       uint64_t* &indices,
                                       const Ogre::Vector3 &position,
                                       const Ogre::Quaternion &orient,
                                       const Ogre::Vector3 &scale);

      /// \brief Print scene graph.
      /// \param[in] _prefix String to prefix each line of output with.
      /// \param[in] _node The Ogre Node to print.
      private: void PrintSceneGraphHelper(const std::string &_prefix,
                                          Ogre::Node *_node);

      /// \brief Called when a scene message is received on the
      /// ~/scene topic
      /// \param[in] _msg The message.
      private: void OnScene(ConstScenePtr &_msg);

      /// \brief Response callback
      /// \param[in] _msg The message data.
      private: void OnResponse(ConstResponsePtr &_msg);

      /// \brief Request callback
      /// \param[in] _msg The message data.
      private: void OnRequest(ConstRequestPtr &_msg);

      /// \brief Joint message callback.
      /// \param[in] _msg The message data.
      private: void OnJointMsg(ConstJointPtr &_msg);

      /// \brief Sensor message callback.
      /// \param[in] _msg The message data.
      private: bool ProcessSensorMsg(ConstSensorPtr &_msg);

      /// \brief Process a joint message.
      /// \param[in] _msg The message data.
      private: bool ProcessJointMsg(ConstJointPtr &_msg);

      /// \brief Process a link message.
      /// \param[in] _msg The message data.
      private: bool ProcessLinkMsg(ConstLinkPtr &_msg);

      /// \brief Proces a scene message.
      /// \param[in] _msg The message data.
      private: void ProcessSceneMsg(ConstScenePtr &_msg);

      /// \brief Process a model message.
      /// \param[in] _msg The message data.
      private: bool ProcessModelMsg(const msgs::Model &_msg);

      /// \brief Scene message callback.
      /// \param[in] _msg The message data.
      private: void OnSensorMsg(ConstSensorPtr &_msg);

      /// \brief Visual message callback.
      /// \param[in] _msg The message data.
      private: void OnVisualMsg(ConstVisualPtr &_msg);

      /// \brief Process a visual message.
      /// \param[in] _msg The message data.
      private: bool ProcessVisualMsg(ConstVisualPtr &_msg);

      /// \brief Light message callback.
      /// \param[in] _msg The message data.
      private: void OnLightMsg(ConstLightPtr &_msg);

      /// \brief Process a light message.
      /// \param[in] _msg The message data.
      private: void ProcessLightMsg(ConstLightPtr &_msg);

      /// \brief Process a request message.
      /// \param[in] _msg The message data.
      private: void ProcessRequestMsg(ConstRequestPtr &_msg);

      /// \brief Selection message callback.
      /// \param[in] _msg The message data.
      private: void OnSelectionMsg(ConstSelectionPtr &_msg);

      /// \brief Sky message callback.
      /// \param[in] _msg The message data.
      private: void OnSkyMsg(ConstSkyPtr &_msg);

      /// \brief Model message callback.
      /// \param[in] _msg The message data.
      private: void OnModelMsg(ConstModelPtr &_msg);

      /// \brief Pose message callback.
      /// \param[in] _msg The message data.
      private: void OnPoseMsg(ConstPosePtr &_msg);

      /// \brief Skeleton animation callback.
      /// \param[in] _msg The message data.
      private: void OnSkeletonPoseMsg(ConstPoseAnimationPtr &_msg);

      /// \brief Create a new center of mass visual.
      /// \param[in] _msg Message containing the link data.
      /// \param[in] _linkVisual Pointer to the link's visual.
      private: void CreateCOMVisual(ConstLinkPtr &_msg, VisualPtr _linkVisual);

      /// \brief Create a center of mass visual using SDF data.
      /// \param[in] _elem SDF element data.
      /// \param[in] _linkVisual Pointer to the link's visual.
      private: void CreateCOMVisual(sdf::ElementPtr _elem,
                                    VisualPtr _linkVisual);

      /// \brief Name of the scene.
      private: std::string name;

      /// \brief Scene SDF element.
      private: sdf::ElementPtr sdf;

      /// \brief All the cameras.
      private: std::vector<CameraPtr> cameras;

      /// \brief All the user cameras.
      private: std::vector<UserCameraPtr> userCameras;

      /// \brief The ogre scene manager.
      private: Ogre::SceneManager *manager;

      /// \brief A ray query used to locate distances to visuals.
      private: Ogre::RaySceneQuery *raySceneQuery;

      /// \brief All the grids in the scene.
      private: std::vector<Grid *> grids;

      /// \brief Unique ID counter.
      private: static uint32_t idCounter;

      /// \brief The unique ID of this scene.
      private: uint32_t id;

      /// \brief String form of the id.
      private: std::string idString;

      /// \def VisualMsgs_L
      /// \brief List of visual messages.
      typedef std::list<boost::shared_ptr<msgs::Visual const> > VisualMsgs_L;

      /// \brief List of visual messages to process.
      private: VisualMsgs_L visualMsgs;

      /// \def LightMsgs_L.
      /// \brief List of light messages.
      typedef std::list<boost::shared_ptr<msgs::Light const> > LightMsgs_L;

      /// \brief List of light message to process.
      private: LightMsgs_L lightMsgs;

      /// \def PoseMsgs_L.
      /// \brief List of messages.
      typedef std::list<boost::shared_ptr<msgs::Pose const> > PoseMsgs_L;

      /// \brief List of pose message to process.
      private: PoseMsgs_L poseMsgs;

      /// \def SceneMsgs_L
      /// \brief List of scene messages.
      typedef std::list<boost::shared_ptr<msgs::Scene const> > SceneMsgs_L;

      /// \brief List of scene message to process.
      private: SceneMsgs_L sceneMsgs;

      /// \def JointMsgs_L
      /// \brief List of joint messages.
      typedef std::list<boost::shared_ptr<msgs::Joint const> > JointMsgs_L;

      /// \brief List of joint message to process.
      private: JointMsgs_L jointMsgs;

      /// \def LinkMsgs_L
      /// \brief List of link messages.
      typedef std::list<boost::shared_ptr<msgs::Link const> > LinkMsgs_L;

      /// \brief List of link message to process.
      private: LinkMsgs_L linkMsgs;

      /// \def ModelMsgs_L
      /// \brief List of model messages.
      typedef std::list<boost::shared_ptr<msgs::Model const> > ModelMsgs_L;
      /// \brief List of model message to process.
      private: ModelMsgs_L modelMsgs;

      /// \def SensorMsgs_L
      /// \brief List of sensor messages.
      typedef std::list<boost::shared_ptr<msgs::Sensor const> > SensorMsgs_L;

      /// \brief List of sensor message to process.
      private: SensorMsgs_L sensorMsgs;

      /// \def RequestMsgs_L
      /// \brief List of request messages.
      typedef std::list<boost::shared_ptr<msgs::Request const> > RequestMsgs_L;
      /// \brief List of request message to process.
      private: RequestMsgs_L requestMsgs;

      /// \def Visual_M
      /// \brief Map of visuals and their names.
      typedef std::map<std::string, VisualPtr> Visual_M;

      /// \brief Map of all the visuals in this scene.
      private: Visual_M visuals;

      /// \def Light_M
      /// \brief Map of lights
      typedef std::map<std::string, LightPtr> Light_M;

      /// \brief Map of all the lights in this scene.
      private: Light_M lights;

      /// \def SkeletonPoseMsgs_L
      /// \brief List of skeleton messages.
      typedef std::list<boost::shared_ptr<msgs::PoseAnimation const> >
                                                          SkeletonPoseMsgs_L;
      /// \brief List of skeleton message to process.
      private: SkeletonPoseMsgs_L skeletonPoseMsgs;

      /// \brief A message used to select an object.
      private: boost::shared_ptr<msgs::Selection const> selectionMsg;

      /// \brief Mutex to lock the various message buffers.
      private: boost::mutex *receiveMutex;

      /// \brief Communication Node
      private: transport::NodePtr node;

      /// \brief Subscribe to sensor topic
      private: transport::SubscriberPtr sensorSub;

      /// \brief Subscribe to scene topic
      private: transport::SubscriberPtr sceneSub;

      /// \brief Subscribe to the request topic
      private: transport::SubscriberPtr requestSub;

      /// \brief Subscribe to visual topic
      private: transport::SubscriberPtr visSub;

      /// \brief Subscribe to light topics
      private: transport::SubscriberPtr lightSub;

      /// \brief Subscribe to pose updates
      private: transport::SubscriberPtr poseSub;

      /// \brief Subscribe to joint updates.
      private: transport::SubscriberPtr jointSub;

      /// \brief Subscribe to selection updates.
      private: transport::SubscriberPtr selectionSub;

      /// \brief Subscribe to reponses.
      private: transport::SubscriberPtr responseSub;

      /// \brief Subscribe to skeleton pose updates.
      private: transport::SubscriberPtr skeletonPoseSub;

      /// \brief Subscribe to sky updates.
      private: transport::SubscriberPtr skySub;

      /// \brief Subscribe to model info updates
      private: transport::SubscriberPtr modelInfoSub;

      /// \brief Publish light updates.
      private: transport::PublisherPtr lightPub;

      /// \brief Publish requests
      private: transport::PublisherPtr requestPub;

      /// \brief Event connections
      private: std::vector<event::ConnectionPtr> connections;

      /// \brief The top level in our tree of visuals
      private: VisualPtr worldVisual;

      /// \brief Pointer to a visual selected by a user via the GUI.
      private: VisualPtr selectedVis;

      /// \brief Keep around our request message.
      private: msgs::Request *requestMsg;

      /// \brief True if visualizations should be rendered.
      private: bool enableVisualizations;

      /// \brief The heightmap, if any.
      private: Heightmap *heightmap;

      /// \brief All the projectors.
      private: std::map<std::string, Projector *> projectors;

      /// \brief Pointer to the sky.
      public: SkyX::SkyX *skyx;

      /// \brief Controls the sky.
      private: SkyX::BasicController *skyxController;
    };
    /// \}
  }
}
#endif
