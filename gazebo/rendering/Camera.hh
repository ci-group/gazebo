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
/* Desc: A persepective OGRE Camera
 * Author: Nate Koenig
 * Date: 15 July 2003
 */

#ifndef _RENDERING_CAMERA_HH_
#define _RENDERING_CAMERA_HH_

#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <list>
#include <vector>
#include <deque>

#include "common/Event.hh"
#include "common/Time.hh"

#include "math/Angle.hh"
#include "math/Pose.hh"
#include "math/Plane.hh"
#include "math/Vector2i.hh"

#include "msgs/MessageTypes.hh"
#include "rendering/RenderTypes.hh"
#include "sdf/sdf.hh"

// Forward Declarations
namespace Ogre
{
  class Texture;
  class RenderTarget;
  class Camera;
  class Viewport;
  class SceneNode;
  class AnimationState;
  class CompositorInstance;
}

namespace gazebo
{
  /// \ingroup gazebo_rendering
  /// \brief Rendering namespace
  namespace rendering
  {
    class MouseEvent;
    class ViewController;
    class Scene;

    /// \addtogroup gazebo_rendering Rendering
    /// \brief A set of rendering related class, functions, and definitions
    /// \{

    /// \class Camera Camera.hh rendering/rendering.hh
    /// \brief Basic camera sensor
    ///
    /// This is the base class for all cameras.
    class Camera : public boost::enable_shared_from_this<Camera>
    {
      /// \brief Constructor
      /// \param[in] _namePrefix Unique prefix name for the camera.
      /// \param[in] _scene Scene that will contain the camera
      /// \param[in] _autoRender Almost everyone should leave this as true.
      public: Camera(const std::string &_namePrefix, Scene *_scene,
                     bool _autoRender = true);

      /// \brief Destructor
      public: virtual ~Camera();

      /// \brief Load the camera with a set of parmeters
      /// \param[in] _sdf The SDF camera info
      public: void Load(sdf::ElementPtr _sdf);

       /// \brief Load the camera with default parmeters
      public: void Load();

      /// \brief Initialize the camera
      public: void Init();

      /// \brief Set the render Hz rate
      /// \param[in] _hz The Hz rate
      public: void SetRenderRate(double _hz);

      /// \brief Render the camera
      ///
      /// Called after the pre-render signal. This function will generate
      /// camera images
      public: void Render();

      /// \brief Post render
      ///
      /// Called afer the render signal.
      public: virtual void PostRender();

      /// \internal
      /// \brief Update the camera information. This does not render images.
      ///
      /// This function gets called automatically. There is no need for the
      /// average user to call this function.
      public: virtual void Update();

      /// \brief Finalize the camera.
      ///
      /// This function is called before the camera is destructed
      public: void Fini();

      /// \brief Return true if the camera has been initialized
      /// \return True if initialized was successful
      public: inline bool IsInitialized() const {return this->initialized;}

      /// \internal
      /// \brief Set the ID of the window this camera is rendering into.
      /// \param[in] _windowId The window id of the camera.
      public: void SetWindowId(unsigned int _windowId);

      /// \brief Get the ID of the window this camera is rendering into.
      /// \return The ID of the window.
      public: unsigned int GetWindowId() const;

      /// \brief Set the scene this camera is viewing
      /// \param[in] _scene Pointer to the scene
      public: void SetScene(Scene *_scene);

      /// \brief Get the global pose of the camera
      /// \return Pose of the camera in the world coordinate frame
      public: math::Pose GetWorldPose();

      /// \brief Get the camera position in the world
      /// \return The world position of the camera
      public: math::Vector3 GetWorldPosition() const;

      /// \brief Get the camera's orientation in the world
      /// \return The camera's orientation as a math::Quaternion
      public: math::Quaternion GetWorldRotation() const;

      /// \brief Set the global pose of the camera
      /// \param[in] _pose The new math::Pose of the camera
      public: virtual void SetWorldPose(const math::Pose &_pose);

      /// \brief Set the world position
      /// \param[in] _pos The new position of the camera
      public: void SetWorldPosition(const math::Vector3 &_pos);

      /// \brief Set the world orientation
      /// \param[in] _quat The new orientation of the camera
      public: void SetWorldRotation(const math::Quaternion &_quat);

      /// \brief Translate the camera
      /// \param[in] _direction The translation vector
      public: void Translate(const math::Vector3 &_direction);

      /// \brief Rotate the camera around the yaw axis
      /// \param[in] _angle Rotation amount
      public: void RotateYaw(math::Angle _angle);

      /// \brief Rotate the camera around the pitch axis
      /// \param[in] _angle Pitch amount
      public: void RotatePitch(math::Angle _angle);

      /// \brief Set the clip distances
      /// \param[in] _near Near clip distance in meters
      /// \param[in] _far Far clip distance in meters
      public: void SetClipDist(float _near, float _far);

      /// \brief Set the camera FOV (horizontal)
      /// \param[in] _radians Horizontal field of view
      public: void SetHFOV(math::Angle _angle);

      /// \brief Get the camera FOV (horizontal)
      /// \return The horizontal field of view
      public: math::Angle GetHFOV() const;

      /// \brief Get the camera FOV (vertical)
      /// \return The vertical field of view
      public: math::Angle GetVFOV() const;

      /// \brief Set the image size
      /// \param[in] _w Image width
      /// \param[in] _h Image height
      public: void SetImageSize(unsigned int _w, unsigned int _h);

      /// \brief Set the image height
      /// \param[in] _w Image width
      public: void SetImageWidth(unsigned int _w);

      /// \brief Set the image height
      /// \param[in] _h Image height
      public: void SetImageHeight(unsigned int _h);

      /// \brief Get the width of the image
      /// \return Image width
      public: unsigned int GetImageWidth() const;

      /// \brief Get the width of the off-screen render texture
      /// \return Render texture width
      public: unsigned int GetTextureWidth() const;

      /// \brief Get the height of the image
      /// \return Image height
      public: unsigned int GetImageHeight() const;

      /// \brief Get the depth of the image
      /// \return Depth of the image
      public: unsigned int GetImageDepth() const;

      /// \brief Get the height of the image
      /// \return String representation of the image format.
      public: std::string GetImageFormat() const;

      /// \brief Get the height of the off-screen render texture
      /// \return Render texture height
      public: unsigned int GetTextureHeight() const;

      /// \brief Get the image size in bytes
      /// \return Size in bytes
      public: size_t GetImageByteSize() const;

      /// \brief Calculate image byte size base on a few parameters.
      /// \param[in] _width Width of an image
      /// \param[in] _height Height of an image
      /// \param[in] _format Image format
      /// \return Size of an image based on the parameters
      public: static size_t GetImageByteSize(unsigned int _width,
                                             unsigned int _height,
                                             const std::string &_format);

      /// \brief Get the Z-buffer value at the given image coordinate.
      /// \param[in] _x Image coordinate; (0, 0) specifies the top-left corner.
      /// \param[in] _y Image coordinate; (0, 0) specifies the top-left corner.
      /// \returns Image z value; note that this is abitrarily scaled and
      /// is @e not the same as the depth value.
      public: double GetZValue(int _x, int _y);

      /// \brief Get the near clip distance
      /// \return Near clip distance
      public: double GetNearClip();

      /// \brief Get the far clip distance
      /// \return Far clip distance
      public: double GetFarClip();

      /// \brief Enable or disable saving
      /// \param[in] _enable Set to True to enable saving of frames
      public: void EnableSaveFrame(bool _enable);

      /// \brief Set the save frame pathname
      /// \param[in] _pathname Directory in which to store saved image frames
      public: void SetSaveFramePathname(const std::string &_pathname);

      /// \brief Save the last frame to disk
      /// \param[in] _filename File in which to save a single frame
      /// \return True if saving was successful
      public: bool SaveFrame(const std::string &_filename);

      /// \brief Get a pointer to the ogre camera
      /// \return Pointer to the OGRE camera
      public: Ogre::Camera *GetOgreCamera() const;

      /// \brief Get a pointer to the Ogre::Viewport
      /// \return Pointer to the Ogre::Viewport
      public: Ogre::Viewport *GetViewport() const;

      /// \brief Get the viewport width in pixels
      /// \return The viewport width
      public: unsigned int GetViewportWidth() const;

      /// \brief Get the viewport height in pixels
      /// \return The viewport height
      public: unsigned int GetViewportHeight() const;

      /// \brief Get the viewport up vector
      /// \return The viewport up vector
      public: math::Vector3 GetUp();

      /// \brief Get the viewport right vector
      /// \return The viewport right vector
      public: math::Vector3 GetRight();

      /// \brief Get the average FPS
      /// \return The average frames per second
      public: virtual float GetAvgFPS() {return 0;}

      /// \brief Get the triangle count
      /// \return The current triangle count
      public: virtual unsigned int GetTriangleCount() {return 0;}

      /// \brief Set the aspect ratio
      /// \param[in] _ratio The aspect ratio (width / height) in pixels
      public: void SetAspectRatio(float _ratio);

      /// \brief Get the apect ratio
      /// \return The aspect ratio (width / height) in pixels
      public: float GetAspectRatio() const;

      /// \brief Set the camera's scene node
      /// \param[in] _node The scene nodes to attach the camera to
      public: void SetSceneNode(Ogre::SceneNode *_node);

      /// \brief Get the camera's scene node
      /// \return The scene node the camera is attached to
      public: Ogre::SceneNode *GetSceneNode() const;

      /// \brief Get a pointer to the image data
      ///
      /// Get the raw image data from a camera's buffer.
      /// \param[in] _i Index of the camera's texture (0 = RGB, 1 = depth).
      /// \return Pointer to the raw data, null if data is not available.
      public: virtual const unsigned char *GetImageData(unsigned int i = 0);

      /// \brief Get the camera's name
      /// \return The name of the camera
      public: std::string GetName() const;

      /// \brief Set the camera's name
      /// \param[in] _name New name for the camera
      public: void SetName(const std::string &_name);

      /// \brief Toggle whether to view the world in wireframe
      public: void ToggleShowWireframe();

      /// \brief Set whether to view the world in wireframe
      /// \param[in] _s Set to True to render objects as wireframe
      public: void ShowWireframe(bool _s);

      /// \brief Get a world space ray as cast from the camera
      /// through the viewport
      /// \param[in] _screenx X coordinate in the camera's viewport, in pixels.
      /// \param[in] _screeny Y coordinate in the camera's viewport, in pixels.
      /// \param[out] _origin Origing in the world coordinate frame of the
      /// resulting ray
      /// \param[out] _dir Direction of the resulting ray
      public: void GetCameraToViewportRay(int _screenx, int _screeny,
                                          math::Vector3 &_origin,
                                          math::Vector3 &_dir);

      /// \brief Set whether to capture data
      /// \param[in] _value Set to true to capture data into a memory buffer.
      public: void SetCaptureData(bool _value);

      /// \brief Set the render target
      /// \param[in] _textureName Name of the new render texture
      public: void CreateRenderTexture(const std::string &_textureName);

      /// \brief Get the scene this camera is in
      /// \return Pointer to scene containing this camera
      public: Scene *GetScene() const;

      /// \brief Get point on a plane
      /// \param[in] _x X cooridnate in camera's viewport, in pixels
      /// \param[in] _y Y cooridnate in camera's viewport, in pixels
      /// \param[in] _plane Plane on which to find the intersecting point
      /// \param[out] _result Point on the plane
      /// \return True if a valid point was found
      public: bool GetWorldPointOnPlane(int _x, int _y,
                  const math::Plane &_plane, math::Vector3 &_result);

      /// \brief Set the camera's render target
      /// \param[in] _target Pointer to the render target
      public: virtual void SetRenderTarget(Ogre::RenderTarget *_target);

      /// \brief Attach the camera to a scene node
      /// \param[in] _visualName Name of the visual to attach the camera to
      /// \param[in] _inheritOrientation True means camera acquires the visual's
      /// orientation
      /// \param[in] _minDist Minimum distance the camera is allowed to get to
      /// the visual
      /// \param[in] _maxDist Maximum distance the camera is allowd to get from
      /// the visual
      public: void AttachToVisual(const std::string &_visualName,
                  bool _inheritOrientation,
                  double _minDist = 0.0, double _maxDist = 0.0);

      /// \brief Set the camera to track a scene node
      /// \param[in] _visualName Name of the visual to track
      public: void TrackVisual(const std::string &_visualName);

      /// \brief Get the render texture
      /// \return Pointer to the render texture
      public: Ogre::Texture *GetRenderTexture() const;

      /// \brief Get the camera's direction vector
      /// \return Direction the camera is facing
      public: math::Vector3 GetDirection() const;

      /// \brief Connect a to the new image signal
      /// \param[in] _subscriber Callback that is called when a new image is
      /// generated
      /// \return A pointer to the connection. This must be kept in scope.
      public: template<typename T>
              event::ConnectionPtr ConnectNewImageFrame(T _subscriber)
              {return newImageFrame.Connect(_subscriber);}

      /// \brief Disconnect from an image frame
      /// \param[in] _c The connection to disconnect
      public: void DisconnectNewImageFrame(event::ConnectionPtr &_c)
              {newImageFrame.Disconnect(_c);}

      /// \brief Save a frame using an image buffer
      /// \param[in] _image The raw image buffer
      /// \param[in] _width Width of the image
      /// \param[in] _height Height of the image
      /// \param[in] _depth Depth of the image data
      /// \param[in] _format Format the image data is in
      /// \param[in] _filename Name of the file in which to write the frame
      /// \return True if saving was successful
      public: static bool SaveFrame(const unsigned char *_image,
                  unsigned int _width, unsigned int _height, int _depth,
                  const std::string &_format,
                  const std::string &_filename);


      /// \brief Get the last time the camera was rendered
      /// \return Time the camera was last rendered
      public: common::Time GetLastRenderWallTime();

      /// \brief Return true if the visual is within the camera's view
      /// frustum
      /// \param[in] _visual The visual to check for visibility
      /// \return True if the _visual is in the camera's frustum
      public: bool IsVisible(VisualPtr _visual);

      /// \brief Return true if the visual is within the camera's view
      /// frustum
      /// \param[in] _visualName Name of the visual to check for visibility
      /// \return True if the _visual is in the camera's frustum
      public: bool IsVisible(const std::string &_visualName);

      /// \brief Returns true if initialized
      /// \return Ture if the camera is initialized
      public: bool GetInitialized() const;

      /// \brief Move the camera to a position. This is an animated motion
      /// \param[in] _pose End position of the camera
      /// \param[in] _time Duration of the camera's movement
      public: virtual bool MoveToPosition(const math::Pose &_pose,
                                          double _time);

      /// \brief Move the camera to a series of positions
      /// \param[in] _pts Vector of poses to move to
      /// \param[in] _time Duration of the entire move
      /// \param[in] _onComplete Callback that is called when the move is
      /// complete
      public: bool MoveToPositions(const std::vector<math::Pose> &_pts,
                                   double _time,
                                   boost::function<void()> _onComplete = NULL);

      /// \brief Implementation of the render call
      protected: virtual void RenderImpl();

      /// \brief Implementation of the Camera::TrackVisual call
      /// \param[in] _visualName Name of the visual to track
      /// \return True if able to track the visual
      protected: bool TrackVisualImpl(const std::string &_visualName);

      /// \brief Set the camera to track a scene node
      /// \param[in] _visual The visual to track
      /// \return True if able to track the visual
      protected: virtual bool TrackVisualImpl(VisualPtr _visual);

      /// \brief Attach the camera to a scene node
      /// \param[in] _visualName Name of the visual to attach the camera to
      /// \param[in] _inheritOrientation True means camera acquires the visual's
      /// orientation
      /// \param[in] _minDist Minimum distance the camera is allowed to get to
      /// the visual
      /// \param[in] _maxDist Maximum distance the camera is allowd to get from
      /// the visual
      /// \return True on success
      protected: virtual bool AttachToVisualImpl(const std::string &_name,
                     bool _inheritOrientation,
                     double _minDist = 0, double _maxDist = 0);

      /// \brief Attach the camera to a visual
      /// \param[in] _visual The visual to attach the camera to
      /// \param[in] _inheritOrientation True means camera acquires the visual's
      /// orientation
      /// \param[in] _minDist Minimum distance the camera is allowed to get to
      /// the visual
      /// \param[in] _maxDist Maximum distance the camera is allowd to get from
      /// the visual
      /// \return True on success
      protected: virtual bool AttachToVisualImpl(VisualPtr _visual,
                     bool _inheritOrientation,
                     double _minDist = 0, double _maxDist = 0);

      /// \brief if user requests bayer image, post process rgb from ogre
      ///        to generate bayer formats
      /// \param[in] _dst Destination buffer for the image data
      /// \param[in] _src Source image buffer
      /// \param[in] _format Format of the source buffer
      /// \param[in] _width Image width
      /// \param[in] _height Image height
      private: void ConvertRGBToBAYER(unsigned char *_dst, unsigned char *_src,
                   std::string _format, int _width, int _height);

      /// \brief Set the clip distance based on stored SDF values
      private: void SetClipDist();

      /// \brief Get the OGRE image pixel format in
      /// \param[in] _format The Gazebo image format
      /// \return Integer representation of the Ogre image format
      private: static int GetOgrePixelFormat(const std::string &_format);

      /// \brief Get the next frame filename based on SDF parameters
      /// \return The frame's filename
      protected: std::string GetFrameFilename();

      /// \brief Create the ogre camera
      private: void CreateCamera();

      protected: std::string name;

      protected: sdf::ElementPtr sdf;

      protected: unsigned int windowId;

      protected: unsigned int textureWidth, textureHeight;

      protected: Ogre::Camera *camera;
      protected: Ogre::Viewport *viewport;
      protected: Ogre::SceneNode *sceneNode;
      protected: Ogre::SceneNode *pitchNode;

      // Info for saving images
      protected: unsigned char *saveFrameBuffer;
      protected: unsigned char *bayerFrameBuffer;
      protected: unsigned int saveCount;

      protected: int imageFormat;
      protected: int imageWidth, imageHeight;

      protected: Ogre::RenderTarget *renderTarget;

      protected: Ogre::Texture *renderTexture;

      private: static unsigned int cameraCounter;
      private: unsigned int myCount;

      protected: bool captureData;

      protected: bool newData;

      protected: common::Time lastRenderWallTime;

      protected: Scene *scene;

      protected: event::EventT<void(const unsigned char *,
                     unsigned int, unsigned int, unsigned int,
                     const std::string &)> newImageFrame;

      protected: std::vector<event::ConnectionPtr> connections;
      protected: std::list<msgs::Request> requests;
      private: friend class Scene;

      private: sdf::ElementPtr imageElem;

      protected: bool initialized;
      private: VisualPtr trackedVisual;

      protected: Ogre::AnimationState *animState;
      protected: common::Time prevAnimTime;
      protected: boost::function<void()> onAnimationComplete;

      private: Ogre::CompositorInstance *dsGBufferInstance;
      private: Ogre::CompositorInstance *dsMergeInstance;

      private: Ogre::CompositorInstance *dlGBufferInstance;
      private: Ogre::CompositorInstance *dlMergeInstance;

      private: Ogre::CompositorInstance *ssaoInstance;

      private: std::deque<std::pair<math::Pose, double> > moveToPositionQueue;
    };
    /// \}
  }
}
#endif
