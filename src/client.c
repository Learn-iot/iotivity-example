//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
// Copyright 2016 Samsung Electronics France SAS All Rights Reserved.
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <octypes.h>
#include <ocstack.h>
#include <logger.h>

#define TAG ("occlient")

static OCConnectivityType gConnectivityType = CT_DEFAULT;
static OCQualityOfService qos = OC_HIGH_QOS;

#define DEFAULT_CONTEXT_VALUE 0x99
static const char * requestUri = "/a/light";

int gQuitFlag = 0;
static OCDevAddr gDestination;
static char discoveryAddr[100];

typedef struct LIGHTRESOURCE
{
    OCResourceHandle handle;
    bool state;
} LightResource;

static LightResource Light;


OCStackApplicationResult postReqCB(void *ctx,
                                   OCDoHandle handle,
                                   OCClientResponse *clientResponse)
{
    if (ctx == (void*)DEFAULT_CONTEXT_VALUE)
    {
        OIC_LOG(INFO, TAG, "Callback Context for POST recvd successfully");
    }

    if (clientResponse)
    {
        OIC_LOG(INFO, TAG, "=============> Post Response");
    }
    else
    {
        OIC_LOG_V(INFO, TAG, "postReqCB received Null clientResponse");
    }
    return OC_STACK_DELETE_TRANSACTION;
}

OCPayload* putPayload()
{
    OCRepPayload* payload = (OCRepPayload*) OCRepPayloadCreate();

    if (!payload)
    {
        OIC_LOG(INFO,TAG, "Failed to create put payload object");
        exit(1);
    }

    Light.state = ! Light.state;
    OCRepPayloadSetPropBool(payload, "state", Light.state);

    return (OCPayload*) payload;
}

// This is a function called back when a device is discovered
OCStackApplicationResult handleDiscovery(void* ctx,
                                         OCDoHandle handle,
                                         OCClientResponse * clientResponse)
{
	OCStackResult result = OC_STACK_OK;

	if (ctx == (void*) DEFAULT_CONTEXT_VALUE)
    {
        OIC_LOG(INFO,TAG,"Callback Context for DISCOVER query recvd successfully");
    }

	if (clientResponse == NULL)
    {
        OIC_LOG(ERROR,TAG,"Payload is NULL, No resources found");
        return   OC_STACK_DELETE_TRANSACTION;
    }

	if (ctx == (void*)DEFAULT_CONTEXT_VALUE)
    {
        OIC_LOG(INFO, TAG, "Callback Context for GET query recvd successfully");
    }


    OIC_LOG_V(INFO, TAG,
              "Device =============> Discovered @ %s:%d",
              clientResponse->devAddr.addr,
              clientResponse->devAddr.port);

	OIC_LOG_V(INFO, TAG, "SEQUENCE NUMBER: %d", clientResponse->sequenceNumber);
	OIC_LOG(INFO, TAG, "=============> Get Response");

    gDestination = clientResponse->devAddr;
    gConnectivityType = clientResponse->connType;

    OCDiscoveryPayload *payload = (OCDiscoveryPayload*) clientResponse->payload;
    if (!payload)
    {
        return OC_STACK_DELETE_TRANSACTION;
    }
    OCResourcePayload *resource = (OCResourcePayload*) payload->resources;

    while (resource)
    {
        if(resource->uri)
        {
            OIC_LOG_V(INFO, TAG, "uri: %s", resource->uri);
        }
        resource = resource->next;
    }
    Light.handle = handle;
	return OC_STACK_KEEP_TRANSACTION;
}

loop()
{

	while (!gQuitFlag)
    {
        printf("iterate: %d\n", Light.state);

        if (OCProcess() != OC_STACK_OK)
        {
            OIC_LOG(ERROR, TAG, "OCStack process error");
            return 0;
        }

        OCCallbackData cbData;
        cbData.cb = postReqCB;
        cbData.context = (void*)DEFAULT_CONTEXT_VALUE;
        cbData.cd = NULL;

        OCStackResult ret;
        OCMethod method = OC_REST_POST;
        OCRepPayload* payload = putPayload();
        OCDoHandle handle;

        OIC_LOG_V(INFO,TAG,"loop: about to %d",method);
        ret = OCDoResource(&handle, method, requestUri, &gDestination,
                           (OCPayload*) payload,
                           gConnectivityType, qos, &cbData, NULL,0);

        if (ret != OC_STACK_OK)
        {
            OIC_LOG_V(ERROR, TAG, "OCDoResource returns error %d with method %d", ret, method);
        }
        sleep(1);
    }

}


setup()
{
	OCStackResult ret;
	char queryUri[MAX_QUERY_LENGTH] = { 0 };


	/* Initialize OCStack*/
	if (OCInit(NULL, 0, OC_CLIENT) != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack init error");
        return 0;
    }

	OCCallbackData cbData;
	cbData.cb = handleDiscovery;
	cbData.context = (void*)DEFAULT_CONTEXT_VALUE;
	cbData.cd = NULL;

	/* Start a discovery query*/
    snprintf(queryUri, sizeof (queryUri), "%s", OC_RSRVD_WELL_KNOWN_URI);
    
	ret = OCDoResource(NULL, OC_REST_DISCOVER, queryUri, NULL, 0, gConnectivityType,
                       qos, &cbData, NULL, 0);
    
	if (ret != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack resource error");
    }

	return ret;
}

/* SIGINT handler: set gQuitFlag to 1 for graceful termination */
void handleSigInt(int signum)
{
	if (signum == SIGINT)
    {
        gQuitFlag = 1;
    }
}

int main()
{
	setup();

	signal(SIGINT, handleSigInt);
	// Break from loop with Ctrl+C
	OIC_LOG(INFO, TAG, "Entering occlient main loop...");

	loop();

	OIC_LOG(INFO, TAG, "Exiting occlient main loop...");

	if (OCStop() != OC_STACK_OK)
    {
    
        OIC_LOG(ERROR, TAG, "OCStack stop error");
    }

	return 0;
}

