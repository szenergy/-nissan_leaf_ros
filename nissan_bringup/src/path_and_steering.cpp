// publishes nav_msgs/Path and steering marker for rviz

#include <math.h>
#include <vector>
#include <ros/ros.h>
#include <nav_msgs/Path.h>
#include <visualization_msgs/Marker.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PointStamped.h>
#include <std_msgs/Float32.h>

double wheelbase = 2.7; // TODO use param vehicle_model_wheelbase instead of constant
double steering_angle;
double map_gyor_0_x = 697237.0, map_gyor_0_y = 5285644.0;
double map_zala_0_x = 639770.0, map_zala_0_y = 5195040.0;
int path_size, hz;
bool first_run = true, publish_steer_marker;
ros::Publisher marker_pub, path1_pub, path2_pub;
nav_msgs::Path path1, path2;
geometry_msgs::PoseStamped actual_pose1, actual_pose2;
int location = 0;
namespace locations // add when new locations appear
{
    enum LOCATIONS
    {
        DEFAULT = 0,
        GYOR,
        ZALA,
        NOT_USED_YET
    };
}

int getLocation(const geometry_msgs::PoseStamped &pose)
{
    double distance_from_gyor = std::sqrt(std::pow(pose.pose.position.x - map_gyor_0_x, 2) + std::pow(pose.pose.position.y - map_gyor_0_y, 2));
    double distance_from_zala = std::sqrt(std::pow(pose.pose.position.x - map_zala_0_x, 2) + std::pow(pose.pose.position.y - map_zala_0_y, 2));
    // ROS_INFO_STREAM("distance_from_gyor: " << distance_from_gyor << " distance_from_zala: " << distance_from_zala);
    if (distance_from_gyor < 50000.0)
    { // 50 km from Gyor
        ROS_INFO_STREAM("Welcome to Gyor");
        return locations::GYOR;
    }
    else if (distance_from_zala < 50000.0)
    { // 50 km from Zalaegerszeg
        ROS_INFO_STREAM("Welcome to Zala");
        return locations::ZALA;
    }
    else
    {
        ROS_INFO_STREAM("Default map");
        return locations::DEFAULT;
    }
}

// Callback for steering wheel messages
void vehicleSteeringCallback(const std_msgs::Float32 &status_msg)
{
    steering_angle = status_msg.data * M_PI / 180; // deg2rad
}

// Callback for pose messages
void vehiclePoseCallback1(const geometry_msgs::PoseStamped &pos_msg)
{
    actual_pose1 = pos_msg;
}

void loop()
{
    if (publish_steer_marker)
    {
        visualization_msgs::Marker steer_marker;
        steer_marker.header.frame_id = "base_link";
        steer_marker.header.stamp = ros::Time::now();
        steer_marker.ns = "steering_path";
        steer_marker.id = 0;
        steer_marker.type = steer_marker.LINE_STRIP;
        steer_marker.action = visualization_msgs::Marker::ADD;
        steer_marker.pose.position.x = 0;
        steer_marker.pose.position.y = 0;
        steer_marker.pose.position.z = 0;
        steer_marker.pose.orientation.x = 0.0;
        steer_marker.pose.orientation.y = 0.0;
        steer_marker.pose.orientation.z = 0.0;
        steer_marker.pose.orientation.w = 1.0;
        steer_marker.scale.x = 0.6;
        steer_marker.color.r = 0.94f;
        steer_marker.color.g = 0.83f;
        steer_marker.color.b = 0.07f;
        steer_marker.color.a = 1.0;
        steer_marker.lifetime = ros::Duration();
        double marker_pos_x = 0.0, marker_pos_y = 0.0, theta = 0.0;
        for (int i = 0; i < 100; i++)
        {
            marker_pos_x += 0.01 * 10 * cos(theta);
            marker_pos_y += 0.01 * 10 * sin(theta);
            theta += 0.01 * 10 / wheelbase * tan(steering_angle);
            geometry_msgs::Point p;
            p.x = marker_pos_x;
            p.y = marker_pos_y;
            steer_marker.points.push_back(p);
        }
        marker_pub.publish(steer_marker);
        steer_marker.points.clear();
    }
    geometry_msgs::PoseStamped pose1;
    std::string current_map = "empty";
    pose1.header.stamp = ros::Time::now();
    if ((actual_pose1.pose.position.x > 0.001 || actual_pose1.pose.position.x < -0.001) && !isnan(actual_pose1.pose.position.y) && !isinf(actual_pose1.pose.position.y))
    {
        if (first_run)
        {
            location = getLocation(actual_pose1);
            first_run = false;
        }
        switch (location)
        {
        case locations::DEFAULT:
            pose1.pose.position = actual_pose1.pose.position;
            current_map = "map";
            break;
        case locations::GYOR:
            pose1.pose.position.x = actual_pose1.pose.position.x - map_gyor_0_x;
            pose1.pose.position.y = actual_pose1.pose.position.y - map_gyor_0_y;
            current_map = "map_gyor_0";
            break;
        case locations::ZALA:
            pose1.pose.position.x = actual_pose1.pose.position.x - map_zala_0_x;
            pose1.pose.position.y = actual_pose1.pose.position.y - map_zala_0_y;
            current_map = "map_zala_0";
            break;
        }
        pose1.header.frame_id = current_map;
        path1.header.frame_id = current_map;
        pose1.pose.orientation = actual_pose1.pose.orientation;
        path1.poses.push_back(pose1);
        path1.header.stamp = ros::Time::now();
    }
    // keep only the last n (path_size) path message
    if (path1.poses.size() > path_size)
    {
        int shift = path1.poses.size() - path_size;
        path1.poses.erase(path1.poses.begin(), path1.poses.begin() + shift);
    }
    if(!first_run){
        path1_pub.publish(path1);
    }
}

int main(int argc, char **argv)
{
    std::string pose_topic1, marker_topic, path_topic1;
    ros::init(argc, argv, "speed_control");
    ros::NodeHandle n;
    ros::NodeHandle n_private("~");
    n_private.param<std::string>("pose_topic1", pose_topic1, "/current_pose");
    n_private.param<std::string>("path_topic1", path_topic1, "/marker_path1");
    n_private.param<std::string>("marker_topic", marker_topic, "/marker_steering");
    n_private.param<int>("path_size", path_size, 100);
    n_private.param<int>("hz", hz, 20);
    n_private.param<bool>("publish_steer_marker", publish_steer_marker, true);
    ros::Subscriber sub_steer = n.subscribe("/wheel_angle_deg", 1, vehicleSteeringCallback);
    ros::Subscriber sub_current_pose1 = n.subscribe(pose_topic1, 1, vehiclePoseCallback1);
    if (publish_steer_marker)
    {
        marker_pub = n.advertise<visualization_msgs::Marker>(marker_topic, 1);
        ROS_INFO_STREAM("Node started: " << ros::this_node::getName() << " subscribed: " << pose_topic1 << " publishing: " << marker_topic << " " << path_topic1);
    }
    else
    {
        ROS_INFO_STREAM("Node started: " << ros::this_node::getName() << " subscribed: " << pose_topic1);
    }
    path1_pub = n.advertise<nav_msgs::Path>(path_topic1, 1);
    ros::Rate rate(hz); // ROS Rate
    while (ros::ok())
    {
        loop();
        rate.sleep();
        ros::spinOnce();
    }

    return 0;
}