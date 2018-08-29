from flask import Flask, json, request
from arduinoControl import ArduinoControl

arduino = ArduinoControl()
app = Flask(__name__)

@app.route("/")
def home():
    return "Use: /getStatus"

# Delete this crap! Just for faking the Arduino
@app.route("/forceDone")
def forceDone():
    arduino.forceStatus("done")
    return "SFERTSCH!"

@app.route("/getLastNStatus")
def getLastNStatus():
    return arduino.getLastNStatus()

@app.route("/createNewOrder", methods=["POST"])
def createNewOrder():
    newOrder = request.get_json()
    arduino.newOrder(newOrder)
    return "Order received"

@app.route("/getStatus")
def getStatus():
    return arduino.getStatus()

@app.route("/stopOrder")
def stopOrder():
    arduino.stopOrder()
    return "Order stopped"

def main():
    print("Starting Webservice...")
    app.run(host="0.0.0.0", port="5550", debug=False, threaded=True)
    print("Webservice stopped")

    arduino.stop()
    print("Exiting...")

if __name__ == "__main__":
    main()
