/*
 * Copyright (C) 2014-2015 Open Source Robotics Foundation
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

#include "gazebo/gui/model/SchematicViewWidget.hh"
#include "gazebo/gui/model/SchematicViewWidget_TEST.hh"

using namespace gazebo;

/////////////////////////////////////////////////
void SchematicViewWidget_TEST::AddRemove()
{
  gui::SchematicViewWidget *svWidget = new gui::SchematicViewWidget();
  QVERIFY(svWidget);

  // add nodes
  QCOMPARE(svWidget->GetNodeCount(), 0u);
  svWidget->AddNode("node_a");
  QCOMPARE(svWidget->GetNodeCount(), 1u);
  svWidget->AddNode("node_b");
  QCOMPARE(svWidget->GetNodeCount(), 2u);
  svWidget->AddNode("node_c");
  QCOMPARE(svWidget->GetNodeCount(), 3u);
  svWidget->AddNode("node_d");
  QCOMPARE(svWidget->GetNodeCount(), 4u);
  // remove node
  svWidget->RemoveNode("node_d");
  QCOMPARE(svWidget->GetNodeCount(), 3u);
  // add it back
  svWidget->AddNode("node_d");
  QCOMPARE(svWidget->GetNodeCount(), 4u);

  // add edges
  QCOMPARE(svWidget->GetEdgeCount(), 0u);
  svWidget->AddEdge("id_0", "edge_0", "node_a", "node_b");
  QCOMPARE(svWidget->GetEdgeCount(), 1u);
  svWidget->AddEdge("id_1", "edge_1", "node_b", "node_c");
  QCOMPARE(svWidget->GetEdgeCount(), 2u);
  svWidget->AddEdge("id_2", "edge_2", "node_a", "node_c");
  QCOMPARE(svWidget->GetEdgeCount(), 3u);
  // remove edge
  svWidget->RemoveEdge("id_2");
  QCOMPARE(svWidget->GetEdgeCount(), 2u);
  // add it back
  svWidget->AddEdge("id_2", "edge_2", "node_a", "node_c");
  QCOMPARE(svWidget->GetEdgeCount(), 3u);

  delete svWidget;
}

// Generate a main function for the test
QTEST_MAIN(SchematicViewWidget_TEST)
