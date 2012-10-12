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

#include "gui/Actions.hh"

QAction *gazebo::gui::g_arrowAct = 0;
QAction *gazebo::gui::g_translateAct = 0;
QAction *gazebo::gui::g_rotateAct = 0;
QAction *gazebo::gui::g_newAct = 0;
QAction *gazebo::gui::g_openAct = 0;
QAction *gazebo::gui::g_importAct = 0;
QAction *gazebo::gui::g_saveAct = 0;
QAction *gazebo::gui::g_saveAsAct = 0;
QAction *gazebo::gui::g_aboutAct = 0;
QAction *gazebo::gui::g_quitAct = 0;

QAction *gazebo::gui::g_newModelAct = 0;
QAction *gazebo::gui::g_resetModelsAct = 0;
QAction *gazebo::gui::g_resetWorldAct = 0;

QAction *gazebo::gui::g_playAct = 0;
QAction *gazebo::gui::g_pauseAct = 0;
QAction *gazebo::gui::g_stepAct = 0;

QAction *gazebo::gui::g_boxCreateAct = 0;
QAction *gazebo::gui::g_sphereCreateAct = 0;
QAction *gazebo::gui::g_cylinderCreateAct = 0;
QAction *gazebo::gui::g_meshCreateAct = 0;
QAction *gazebo::gui::g_pointLghtCreateAct = 0;
QAction *gazebo::gui::g_spotLghtCreateAct = 0;
QAction *gazebo::gui::g_dirLghtCreateAct = 0;

QAction *gazebo::gui::g_viewGridAct = 0;
QAction *gazebo::gui::g_viewResetAct = 0;
QAction *gazebo::gui::g_viewFullScreenAct = 0;
QAction *gazebo::gui::g_viewFPSAct = 0;
QAction *gazebo::gui::g_viewOrbitAct = 0;
