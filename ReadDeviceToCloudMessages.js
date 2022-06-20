// Copyright (c) Microsoft Corporation.
// Licensed under the MIT Licence.

/*
  This sample demonstrates how to use the Microsoft Azure Event Hubs Client for JavaScript to 
  read messages sent from a device. Please see the documentation for @azure/event-hubs package
  for more details at https://www.npmjs.com/package/@azure/event-hubs

  For an example that uses checkpointing, follow up this sample with the sample in the 
  eventhubs-checkpointstore-blob package on GitHub at the following link:

  https://github.com/Azure/azure-sdk-for-js/blob/master/sdk/eventhub/eventhubs-checkpointstore-blob/samples/javascript/receiveEventsUsingCheckpointStore.js
*/

const express = require("express");

const { EventHubConsumerClient } = require("@azure/event-hubs");

// If using websockets, uncomment the following require statement
// const WebSocket = require("ws");

// If you need proxy support, uncomment the below code to create proxy agent
// const HttpsProxyAgent = require("https-proxy-agent");
// const proxyAgent = new HttpsProxyAgent(proxyInfo);

// Event Hub-compatible endpoint
// az iot hub show --query properties.eventHubEndpoints.events.endpoint --name {your IoT Hub name}
// az iot hub show --query properties.eventHubEndpoints.events.endpoint --name {your IoT Hub name}
const eventHubsCompatibleEndpoint = "*******************************"; 

// Event Hub-compatible name
// az iot hub show --query properties.eventHubEndpoints.events.path --name {your IoT Hub name}
const eventHubsCompatiblePath = "azureblack";

// Primary key for the "service" policy to read messages
// az iot hub policy show --name service --query primaryKey --hub-name {your IoT Hub name}
const iotHubSasKey = "**************************************";

// If you have access to the Event Hub-compatible connection string from the Azure portal, then
// you can skip the Azure CLI commands above, and assign the connection string directly here.
const connectionString = `Endpoint=${eventHubsCompatibleEndpoint};EntityPath=${eventHubsCompatiblePath};SharedAccessKeyName=service;SharedAccessKey=${iotHubSasKey}`;

var printError = function (err) {
  console.log(err.message);
};

var coneVisible = "no";
var printMessages = function (messages) {
  coneVisible = "no";
  for (const message of messages) {
    console.log(JSON.stringify(message.body));
    if (message.body.NEURAL_NETWORK.length > 0) {
      for (detection of message.body.NEURAL_NETWORK) {
        if (detection.label == "cone" && detection.confidence > 0.50) {
          coneVisible = "cone";
        }
        else if (detection.label == "LEGOMan" && detection.confidence > 0.50) {
          coneVisible = "LEGOMan";
        }
      }
    }
  }
};


async function main() {
  console.log("Welcome to the Conference - I'm reading device to cloud messages.");

  // If using websockets, uncomment the webSocketOptions below
  // If using proxy, then set `webSocketConstructorOptions` to { agent: proxyAgent }
  // You can also use the `retryOptions` in the client options to configure the retry policy
  const clientOptions = {
    // webSocketOptions: {
    //   webSocket: WebSocket,
    //   webSocketConstructorOptions: {}
    // }
  };

  // Create the client to connect to the default consumer group of the Event Hub
  const consumerClient = new EventHubConsumerClient("$Default", connectionString, clientOptions);

  // Subscribe to messages from all partitions as below
  // To subscribe to messages from a single partition, use the overload of the same method.
  consumerClient.subscribe({
    processEvents: printMessages,
    processError: printError,
  });
}

main().catch((error) => {
  console.error("Error running sample:", error);
});

//express
const app = express();

//required headers
app.use(function(req, res, next) {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
  next();
});


app.get("/", function(req, res) {
  res.send(coneVisible);
});


// port 3333
let port = process.env.PORT;
if(port == null || port == "") {
 port = 3333;
}

app.listen(port, function() {
 console.log("Success");
});
