/*
 * Copyright 2015 Open Source Robotics Foundation
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

#ifndef _GAZEBO_RESTUI_WIDGET_HH_
#define _GAZEBO_RESTUI_WIDGET_HH_

#include <string>
#include <list>

#include <gazebo/gazebo.hh>
#include <gazebo/util/system.hh>
#include "RestUiLoginDialog.hh"

namespace gazebo
{
  /// \class RestUiWidget RestUiWidget.hh RestUiWidget.hh
  /// \brief REST user interface widget
  class GAZEBO_VISIBLE RestUiWidget : public QWidget
  {
    Q_OBJECT

    /// \brief Constructor
    /// \param[in] _parent Parent widget.
    /// \param[in] _menuTitle Menu title.
    /// \param[in] _loginTitle Login title.
    /// \param[in] _urlLabel Url label.
    /// \param[in] _defaultUrl Default url.
    public: RestUiWidget(QWidget *_parent,
                         const std::string &_menuTitle,
                         const std::string &_loginTitle,
                         const std::string &_urlLabel,
                         const std::string &_defautlUrl);

    /// \brief Destructor
    public: virtual ~RestUiWidget() = default;

    /// \brief QT callback (from the login menu)
    public slots: void Login();

    /// \brief Called before rendering, from the GUI thread this is called from
    /// the plugin's update.
    public: void Update();

    /// \brief Called everytime a response  message is received.
    /// \param[in] _msg Rest error message.
    private: void OnResponse(ConstRestErrorPtr &_msg);

    /// \brief The title to use when displaying dialog/message windows
    private: std::string title;

    /// \brief Pub/sub node to communicate with gzserver.
    private: gazebo::transport::NodePtr node;

    /// \brief Login dialog.
    private: gui::RestUiLoginDialog dialog;

    /// \brief Gazebo topics publisher
    private: gazebo::transport::PublisherPtr pub;

    /// \brief Gazebo topics subscriber.
    private: gazebo::transport::SubscriberPtr sub;

    /// \brief List of unprocessed error messages to be displayed from the gui
    /// thread.
    private: std::list<boost::shared_ptr<const gazebo::msgs::RestError>>
        msgRespQ;
  };
}

#endif
