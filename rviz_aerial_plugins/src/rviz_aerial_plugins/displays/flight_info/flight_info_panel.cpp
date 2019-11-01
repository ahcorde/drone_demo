// Copyright 2019 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rviz_aerial_plugins/displays/flight_info/flight_info_panel.hpp"
#include "rviz_common/load_resource.hpp"

namespace rviz_aerial_plugins
{

namespace displays
{

FlighInfoDisplay::FlighInfoDisplay(QWidget* parent):
 rviz_common::Panel(parent), rviz_ros_node_()
{
  // setIcon(rviz_common::loadPixmap("package://rviz_aerial_plugins/icons/classes/Battery.png"));

  odometry_topic_name_ = "/iris_0/vehicle_odometry";
  attitude_topic_name_ = "/iris_0/vehicle_attitude";
}

FlighInfoDisplay::~FlighInfoDisplay()
{

}

void FlighInfoDisplay::onInitialize()
{
  rviz_ros_node_ = getDisplayContext()->getRosNodeAbstraction();

  compass_widget_ = new CompassWidget();
  adi_widget_ = new ADIWidget();
  vi_widget_ = new VehicleInformationWidget();
  namespace_ = new QComboBox();
  add_namespaces_to_combobox();

  QGridLayout *grid = new QGridLayout;

  grid->addWidget( compass_widget_ , 0, 0);
  grid->addWidget( adi_widget_, 0, 1);
  grid->addWidget( vi_widget_, 1, 0, 1, 2);
  grid->addWidget( namespace_, 2, 0);

  QObject::connect(namespace_, SIGNAL(currentIndexChanged(QString)),this, SLOT(on_changed_namespace(QString)));

  setLayout(grid);

  subcribe2topics();

}

void FlighInfoDisplay::add_namespaces_to_combobox()
{
  auto names_and_namespaces = rviz_ros_node_.lock()->get_raw_node()->get_node_names();

  std::set<std::string> namespaces = get_namespaces(names_and_namespaces);

  namespace_->blockSignals(true);
  namespace_->clear();
  for(auto n: namespaces){
    namespace_->addItem(QString(n.c_str()));
  }
  namespace_->blockSignals(false);
}

void FlighInfoDisplay::on_changed_namespace(const QString& text)
{
  std::string namespace_str(text.toUtf8().constData());

  attitude_topic_name_ = "/" + namespace_str + "/vehicle_attitude";
  odometry_topic_name_ = "/" + namespace_str + "/vehicle_odometry";
  vehicle_attitude_sub_.reset();
  vehicle_odometry_sub_.reset();

  subcribe2topics();
}

void FlighInfoDisplay::subcribe2topics()
{
  vehicle_attitude_sub_ = rviz_ros_node_.lock()->get_raw_node()->
      template create_subscription<px4_msgs::msg::VehicleAttitude>(
        attitude_topic_name_,
      10,
      [this](px4_msgs::msg::VehicleAttitude::ConstSharedPtr msg) {

        geometry_msgs::msg::Quaternion q;
        q.x = msg->q[1];
        q.y = msg->q[2];
        q.z = msg->q[3];
        q.w = msg->q[0];
        double yaw, pitch, roll;
        tf2::getEulerYPR(q, yaw, pitch, roll);
        compass_widget_->setAngle(yaw*180/3.1416);
        compass_widget_->update();
        adi_widget_->setPitch(pitch*180/3.1416);
        adi_widget_->setRoll(-roll*180/3.1416);
        adi_widget_->update();
    });

  vehicle_odometry_sub_ = rviz_ros_node_.lock()->get_raw_node()->
        template create_subscription<px4_msgs::msg::VehicleOdometry>(
          odometry_topic_name_,
        10,
        [this](px4_msgs::msg::VehicleOdometry::ConstSharedPtr msg) {
          vi_widget_->setGroundSpeed(sqrt(msg->vx*msg->vx + msg->vy*msg->vy + msg->vz*msg->vz));
          vi_widget_->setAlt(-msg->z);
      });
}

} // namespace displays

} // namespace rviz_aerial_plugins

#include <pluginlib/class_list_macros.hpp>  // NOLINT
PLUGINLIB_EXPORT_CLASS(rviz_aerial_plugins::displays::FlighInfoDisplay, rviz_common::Panel)
