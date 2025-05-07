# Visualizations
The [example JSON file](visualizations/../example03.json) shows how the data looked as output from the ESP32 exit node of our system -> into the entrance of the cloud in AWS IoT Core. This data was processed in realtime using AWS lambda functions and IoT SiteWise and sent to Grafana for visualization. Multiple packets of data were sent simultaneously and the updates from the system were seen in real-time in Grafana.

![iot-core-mqtt-broker](https://github.com/user-attachments/assets/74363904-9064-404b-ac26-e72fddaddc29)

As part of our project, we used the AWS IoT Core MQTT Test Client to test and verify message delivery between our IoT devices and the cloud. This tool allowed us to publish and subscribe to different MQTT topics in order to simulate and monitor communication. In our case, we created multiple topics, such as “esp/Module/bulk1” through “esp/Module/bulk4”, to represent the different modules in the system. The Figure shows the MQTT client interface. Once published, any messages sent to the subscribed topics from the exit node of the eDAQ appear immediately in the interface, confirming that the data was being received correctly.

# Grafana Dashboard
This is where the KPI report is produced in an interactive way

### Dashboard for Modules
![grafana-module-dashboard](https://github.com/user-attachments/assets/8e873195-3809-4431-b230-d43657b50ef6)

After verifying that messages were being sent successfully, we focused on visualizing the data using Grafana. We set up a dashboard that pulls data from ioT SiteWise and displays it in a more organized and readable format. As shown in the Figure above, the dashboard provides an overview of all modules in the system, with real-time updates on metrics like voltage, current, temperature, and time since last update. This made it easier for us to monitor each module's performance and quickly spot any irregularities. The dashboard also includes graphs and gauges, which helped us understand trends over time.

### Dashboard for Cells
![grafana-cell-dashboard](https://github.com/user-attachments/assets/ecf10432-2906-4592-89b4-ee2909f6f07d)

In addition to the module-level view, we also set up another Grafana dashboard to display more specific data related to the internal components of each module. This allowed us to get a better understanding of how different parts of the system were behaving over time. The above Figure shows how information such as voltage, current, and temperature can be broken down further and viewed more closely. This helped us to observe more data for each module.
