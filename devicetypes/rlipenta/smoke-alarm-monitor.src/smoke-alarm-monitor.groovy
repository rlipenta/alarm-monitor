/**
 *    Smoke Alarm Monitor [Device Handler]
 *        Uses a simple ESP8266 circuit to detect the sound of a (not-so-smart) smoke alarm
 *
 *    Copyright 2017 Ross Lipenta
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *    in compliance with the License. You may obtain a copy of the License at:
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *    on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *    for the specific language governing permissions and limitations under the License.
 *
 */
import groovy.json.JsonSlurper
 
metadata {
    definition (name: "Smoke Alarm Monitor", namespace: "rlipenta", author: "Ross Lipenta") {
        capability "Refresh"
        capability "Sensor"
        capability "Smoke Detector"
                
        command "testAlarm"
    }

    simulator {
    }

    tiles {
        standardTile("alarm", "device.smoke", width: 3, height: 3) {
            state("clear", label: "Clear", icon: "st.alarm.smoke.clear", backgroundColor: "#ffffff", action: "refresh", defaultState: true)
            state("detected", label: "Smoke!", icon: "st.alarm.smoke.smoke", backgroundColor: "#e86d13", action: "refresh")
        }
        valueTile("testAlarm", "device.alarm", width: 3, height: 1, decoration: "flat") {
            state("test", label: "Test / Reset", icon: "st.alarm.smoke.test", action: "testAlarm", defaultState: true)
        }

        main "alarm"
        details(["alarm", "refresh", "testAlarm"])
    }
}

/**
 * Interacts with the Alarm-Monitor device to trigger the alarm
 *
 *     The device exposes a "/test" URL that will toggle the alarm
 *     state. This command simply calls that URL to help test the
 *     connectivity between the device and SmartThings. It could
 *     also be used to clear (or reset) a false alarm.
 */
def testAlarm() {
    log.trace "Executing 'testAlarm'"
        
    def hubAction = new physicalgraph.device.HubAction(
            method: "GET",
            path: "/test",
            headers: [
                Accept: "application/json",
                HOST: getHostAddress()
            ],
            query: []
    )

    log.debug hubAction

    sendHubCommand(hubAction)
}

/**
 * Parses hub action events
 * 
 * @description the event details
 */
def parse(String description) {
    log.trace "Executing 'parse'"
    log.trace "Parse: ${description}"

    def result = []

    def msg = parseLanMessage(description)
    log.trace "    data: ${msg.body}"
    
    if (msg.body) {
        def jsonBody = new groovy.json.JsonSlurper().parseText(msg.body)
        log.trace "jsonBody: ${jsonBody}"
        log.debug "isAlarmTriggered: ${jsonBody.isAlarmTriggered}"
        if (jsonBody.isAlarmTriggered) {
            result << createEvent(name: "smoke", value: "detected", descriptionText: "$device.displayName smoke detected!")
        } else {
            result << createEvent(name: "smoke", value: "clear", descriptionText: "$device.displayName clear")
        }
    }
    
    return result
}

/**
 * Subscribes to any device events
 * 
 *    This is called by the SmartApp when the app updates information
 *    pertaining to the device.
 */
def subscribe() {
    log.trace "Executing 'subscribe'"
    subscribeAction()
}

/**
 * Updates details about the device
 *
 *    This is called by the SmartApp when the app updates information
 *    pertaining to the device
 */
def updated() {
    log.trace "Executing 'updated'"
    unsubscribe()
    subscribe()
}

/**
 * Refreshes the device handler state based on the state of the device
 *
 *    This can be invoked manually or automatically by SmartThings
 *    It will call out to the main URL of the device ("/") and get an
 *    updated status from the device.
 */
def refresh() {
    log.trace "Executing 'refresh'"
        
    log.trace "Device: ${device}"
    
    def hubAction = new physicalgraph.device.HubAction(
            method: "GET",
            path: "/",
            headers: [
                Accept: "application/json",
                HOST: getHostAddress()
            ],
            query: []
    )

    log.debug hubAction

    hubAction
}

/**
 * Subscribes to the event notification feed of the device
 *
 *    Issues a UPNP Subscribe action to the device to supply the
 *    device with a callback URL that the device will call when
 *    an alarm is detected.
 */
private subscribeAction() {
    log.trace "Executing 'subscribeAction'"
    
    def hubIP = device.hub.getDataValue("localIP")
    def hubPort = device.hub.getDataValue("localSrvPortTCP")
    
    def result = new physicalgraph.device.HubAction(
            method: "SUBSCRIBE",
            path: "/event",
            headers: [
                    HOST: getHostAddress(),
                    CALLBACK: "<http://${hubIP}:${hubPort}/notify>",
                    NT: "upnp:event",
                    TIMEOUT: "Second-3600"]
    )
    
    log.debug result
    
    sendHubCommand(result)
}

/**
 * Gets the Host IP and Port of the device
 *
 *  Converts the stored hex representation of the IP and Port
 *  of the device to an IP:Port format suitable for use as a
 *  HOST value in an HTTP request header
 */
private String getHostAddress() {
    def host = convertHexToIP(getDataValue("ip"))
    def port = convertHexToInt(getDataValue("port"))
    return "${host}:${port}"
}

/**
 *  Convert hex (e.g. port number) to decimal number
 *
 *  @hex a hexidecimal number to convert to an integer
 *  @return an integer
 */
private Integer convertHexToInt(hex) {
    return Integer.parseInt(hex,16)
}

/**
 * Convert internal hex representation of IP address to dotted quad
 *
 * @hex a hexidecimal representation of the IP address
 * @return an IP address in dotted quad notation
 */
private String convertHexToIP(hex) {
    return [convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".")
}