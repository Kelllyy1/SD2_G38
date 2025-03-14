Folder for the scheduling algorithms.

The code in this folder has code for 5 ESP32.
The idea behind these codes is to simulate a constant stream of JSON information across the row.

Node_1 : Imitate the entry node this would be receiving from the left side (Data Simulate / Pseudo Data Sim) through UART and sending to the right through UART. This node can also process data designated for it.

Node_5 and Node_9: Imitates normal nodes where it can process and send information (Left and Right  ---will work on a version where it can work up, down, left and right)

Node_13 : Imitate a little like the Exit node but has the functionality to "process" and "upload" (Not an actual process and upload it is more of a string saying it did) I would need to change this in the future.

---- Note ----
I need to work more on the vertical transfer of data , and I need to discuss with team more on HOW to stress test this algorithm

-------- Observations --------
When working on this algorithm it is catered to flow in one direction. I have momentary delays(2 seconds) that shows how the data trickles into the system (This trickle of data comes from the left of the entry node called Pseudo_Data_Sim). Each of the nodes have a queue that should hold about "10 of the packets". How the queue gets populated is through constant listening on both sides to see if there is information comming in once it recieves something it would push the information to the queue. The moment that the queue gets populated it would then trigger the "processTask" which it will check the information to see if it should be sent or processed. If processed it would then immediately send to the right.



------- Changes --------
* Change functionality of Exit_Node
* Implement 4 direction sending of information.
* Work on the pathing function.

---- Need : More ESP32s but I'm currently using all the com ports available on my laptop and PC.

