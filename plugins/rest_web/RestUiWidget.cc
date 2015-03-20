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

#include <curl/curl.h>
#include <QMessageBox>
#include "RestUiWidget.hh"

using namespace gazebo;

/////////////////////////////////////////////////
RestUiWidget::RestUiWidget(QWidget *_parent,
                            const std::string &_menuTitle,
                            const std::string &_loginTitle,
                            const std::string &_urlLabel,
                            const std::string &_defautlUrl)
  : QWidget(_parent),
    title(_menuTitle),
    node(new gazebo::transport::Node()),
    dialog(this, _loginTitle, _urlLabel, _defautlUrl)
{
  node->Init();
  pub = node->Advertise<gazebo::msgs::RestLogin>("/gazebo/rest/rest_login");
  // this for a problem where the server cannot subscribe to the topic
  pub->WaitForConnection();
  sub = node->Subscribe("/gazebo/rest/rest_error",
                        &RestUiWidget::OnResponse,
                        this);
}

/////////////////////////////////////////////////
RestUiWidget::~RestUiWidget()
{
  // no clean up necessary
}

/////////////////////////////////////////////////
void RestUiWidget::Login()
{
  if (dialog.exec() != QDialog::Rejected)
  {
    gazebo::msgs::RestLogin msg;
    msg.set_url(dialog.GetUrl());
    msg.set_username(dialog.GetUsername());
    msg.set_password(dialog.GetPassword());
    pub->Publish(msg);
  }
}

/////////////////////////////////////////////////
void RestUiWidget::OnResponse(ConstRestErrorPtr &_msg )
{
  gzerr << "Error received:" << std::endl;
  gzerr << " type: " << _msg->type() << std::endl;
  gzerr << " msg:  " << _msg->msg() << std::endl;

  // add msg to queue for later processing from
  // the GUI thread
  // msgQueue.push_back(_msg);
  msgRespQ.push_back(_msg);
}

/////////////////////////////////////////////////
void  RestUiWidget::Update()
{
  // Login problem?
  while (!msgRespQ.empty())
  {
    ConstRestErrorPtr msg = msgRespQ.front();
    msgRespQ.pop_front();
    if (msg->type() == "Error")
    {
      QMessageBox::critical(this,
                            tr(this->title.c_str()),
                            tr(msg->msg().c_str()));
    }
    else
    {
      QMessageBox::information(this,
                               tr(this->title.c_str()),
                               tr(msg->msg().c_str()));
    }
  }
}
