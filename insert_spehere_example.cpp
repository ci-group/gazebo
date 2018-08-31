
          sdf::SDF sphereSDF;
          sphereSDF.SetFromString(
                  "<sdf version ='1.5'>\
                     <model name ='sphere'>\
                       <pose>1 0 0 0 0 0</pose>\
                       <link name ='link'>\
                         <pose>0 0 .5 0 0 0</pose>\
                         <collision name ='collision'>\
                           <geometry>\
                             <sphere><radius>0.5</radius></sphere>\
                           </geometry>\
                         </collision>\
                         <visual name ='visual'>\
                           <geometry>\
                             <sphere><radius>0.5</radius></sphere>\
                           </geometry>\
                         </visual>\
                       </link>\
                     </model>\
                   </sdf>");
          // Demonstrate using a custom model name.
          sdf::ElementPtr model = sphereSDF.Root()->GetElement("model");
          model->GetAttribute("name")->SetFromString("unique_sphere");
          robot_model->GetWorld()->InsertModelSDF(sphereSDF);
