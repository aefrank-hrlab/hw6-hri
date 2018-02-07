/*************************************************************/
/* 			   CSE 276B: Human Robot Interaction 			 */
/*   Week 4, HW 6: Non-verbal sensing & multimodal control   */
/*                        Andi Frank                         */
/*************************************************************/


/*************************************************************/
/*							SETUP							 */
/*************************************************************/
// #include <kobuki_msgs/BumperEvent.h> 
#include <geometry_msgs/Twist.h>
#include <ros/ros.h>
#include <cmvision/Blobs.h>
#include <stdio.h>
#include <vector>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <time.h>
#include <math.h>

typedef pcl::PointCloud<pcl::PointXYZ> PointCloud;

// Center of the screen
#define SCREENCENTER 320
#define CENTERTHRESHOLD 10

#define GOAL_Z 0.5 // distance at which we consider the target acquired (units unidentified)
#define GOAL_R 0
#define GOAL_G 255
#define GOAL_B 0


// Target data
struct Target {
	double x;
	double z;	
};
Target target;

/*************************************************************/
/*						 	   FSM						  	 */
/*************************************************************/

// States.
enum Target_State{
	NO_TARGET,
	TARGET_UNCENTERED,
	TARGET_CENTERED,
	TARGET_ACQUIRED
};
Target_State state = NO_TARGET;

void update_state(){
	// Update state.
	 if ( abs(target.x - SCREENCENTER) < CENTERTHRESHOLD) 				// If within threshold of center...
	 	if ( target.z <= GOAL_Z ) 	{ 	state = TARGET_ACQUIRED; 	}		// AND close enough, target acquired!
	 	else 						{ 	state = TARGET_CENTERED; 	}		// BUT not close enough, target centered.
	 else 							{	state = TARGET_UNCENTERED;	}	// Otherwise, target not centered.
}

// ACTIONS
void wander(){}

void aim(){}

void approach(){}

void halt(){}


/*************************************************************/
/*		     	     HARDWARE COMMUNICATION				  	 */
/*************************************************************/

// 							Callbacks

/************************************************************
 * Function Name: 	blobs_callback
 * Inputs:			const cmvision::Blobs&, pointer to Blobs 
 * 				 		object from the /blobs topic.
 * Outputs:			void
 *
 * Description:	@brief: Finds centroid of target color.
 * 				Function reads in blobs_in, checks if the
 * 					blob is the target color, and stores the
 * 					x coordinate of the centroid of the target
 * 					color. State variable is updated to
 *					TARGET_CENTERED or TARGET_UNCENTERED if
 *					needed. This processing is skipped if
 *					state == TARGET_ACQUIRED.
 ***********************************************************/
void blob_callback(const cmvision::Blobs& blobs_in) {

	// @TODO: Change this so only blob_callback or cloud_callback processes blobs at once
	if (state != TARGET_ACQUIRED){ 	// skip processing if target has already been acquired
		double target_sum_x = 0;
		int i;
		for (i = 0; i < blobs_in.blob_count; i++) {
			if (	blobs_in.blobs[i].red 	== 	GOAL_R &&
					blobs_in.blobs[i].green == 	GOAL_G &&
					blobs_in.blobs[i].blue 	== 	GOAL_B )
			{
				 target_sum_x += blobs_in.blobs[i].x; //stores the sum coordinates of the target blobs
			}
		}
		// Find average horizontal position of target color centroid and update target.x.
		 target.x = target_sum_x/i;

		 // Update state.
		 update_state();

	}
}


void cloud_callback(const PointCloud::ConstPtr& cloud){}


/*************************************************************/
/*						    MAIN			  				 */
/*************************************************************/

int main (int argc, char *argv[]){

	// Local variables.
	geometry_msgs::Twist twist;

	// Initiate ROS processes.
	ros::init(argc, argv, "hw6");
	ros::NodeHandle nh;

	// Subscribe to /PointCloud2 topic.
    ros::Subscriber pc_sub = nh.subscribe<PointCloud>("/camera/depth/points", 1, cloud_callback);  

    // Subscribe to /blobs topic .
    ros::Subscriber blob_sub = nh.subscribe("/blobs", 100, blob_callback);

    // Publisher for geometry twist message.
    ros::Publisher vel_pub = nh.advertise<geometry_msgs::Twist>("cmd_vel_mux/input/teleop", 1000);
	
	// Set the loop frequency in Hz.
    ros::Rate fs(10);

    // Denote that setup is complete.
    ROS_INFO("Program started.");

	while(1){
		// control flow
		if 		( state == TARGET_ACQUIRED ) 	{ 	halt();		}
		else if ( state == TARGET_CENTERED ) 	{ 	approach(); }
		else if ( state == TARGET_UNCENTERED ) 	{ 	aim();		}
		else 									{	wander();	}

		// Publish twist message.
		vel_pub.publish(twist);
		// Spin.
		ros::spinOnce();
		// Sleep to sample rate.
		fs.sleep();
	}
}


