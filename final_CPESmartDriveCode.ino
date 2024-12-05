#include <Adafruit_CircuitPlayground.h>
#include <PulseSensorPlayground.h>
#include <math.h>

// Constants and thresholds
#define THRESHOLD 550          // Threshold for pulse sensor detection
#define STRESS_THRESHOLD 30    // RMSSD threshold for stress detection
#define SIZE 10                // Size of IBI differences array
#define s 3                   // size of raw data to smoth
int countDiff;


// Pulse sensor setup
const int PulseSensorPin = 10; // pin for reading analog signal from pulse sensor
PulseSensorPlayground pulseSensor;

class StressAlert {
public:


    void checkStressDetection(float RMSSD, double sdnn) {
        if (RMSSD < STRESS_THRESHOLD && sdnn < STRESS_THRESHOLD) {

            //stressful conditions
            do {
                activateAlert();

            } while (!CircuitPlayground.rightButton()); // give an alert until the user hit right button

            // reset LED and buzzer to signify transition state
            acknowledge();



        }

        // check for intermediate state
        else if ((RMSSD > STRESS_THRESHOLD && RMSSD < 60) && (sdnn > STRESS_THRESHOLD && sdnn < 60))
        {
            // modify alert signals to show intermediate state
            handleIntermediateCondition();

        }
        else
        {
            // normal conditions
            resetAlert();

        }

    }


private:

    // function for providing an alert in case of stress detection
    void activateAlert() {

        Serial.println("Stress detected!");
        CircuitPlayground.playTone(1000, 200); // Play tone for 200ms
        for (int i = 0; i < 10; i++)
        {
            CircuitPlayground.setPixelColor(i, 200, 0, 0); // Set LED to RED
        }

    }

    // function for resetting alert to signify normal state
    void resetAlert() {
        for (int i = 0; i < 10; i++)
        {
            CircuitPlayground.setPixelColor(i, 0, 255, 0); // Set LED to GREEN
        }


    }


    void acknowledge() {

        Serial.println("Alert acknowledged.");

        // set LED to yellow to show transition state
        for (int i = 0; i < 10; i++)
        {
            CircuitPlayground.setPixelColor(i, 200, 200, 0); // Set LED to YELLOW
        }
        countDiff = 0;
        delay(5000); // Delay for 5 seconds


    }

    // function for setting alert signify intermediate state
    void handleIntermediateCondition() {
        for (int i = 0; i < 10; i++)
        {
            CircuitPlayground.setPixelColor(i, 200, 200, 0); // Set LED to YELLOW
        }

    }


};

// Create an instance of the StressAlert class
StressAlert stressAlert;

// initialize valuables
int raw_IBI[s] = { 0, 0, 0 };
int raw_signal[s] = { 0,0,0 };
int IBI = 0;                   // Inter-beat interval
int lastIBI = 0;               // Previous IBI
int IBIDiffs[SIZE];            // Array to store last 10 successive IBI differences
int ibi[SIZE];
//int countDiff = 0;             // Counter for number of differences
float RMSSD = 0;
double sdnn = 0;               // Variable to store calculated RMSSD
int index = 0;

// Function to calculate RMSSD
float computeRMSSD(int IBIDiffs[], int size) {
    float sumOfSquares = 0.0;
    // calculate the sum of differences between consecutive interbeat intervals
    for (int i = 0; i < size; i++) {
        sumOfSquares += pow(IBIDiffs[i], 2);  // Sum of squares of differences
    }
    return sqrt(sumOfSquares / size);      // Return RMSSD
}

// Function to calculate SDNN
double calculateSDNN(int rr[], int size) {
    double meanRR = 0.0;
    for (int i = 0; i < size; i++) {
        meanRR += rr[i]; // sum of interbeat intervals
    }
    meanRR /= size; // mean of interbeat intervals

    double sumSquaredDifferences = 0.0;
    for (int i = 0; i < size; i++) {
        sumSquaredDifferences += pow(rr[i] - meanRR, 2);
    }
    // return standard deviation
    return sqrt(sumSquaredDifferences / (size - 1));
}

// function to calculate moving average to filter noise in the data
double computeSMA(int data[], int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += data[i]; // Sum of the last `size` values
    }
    return sum / (double)size; // Return the average
}

void setup() {
    Serial.begin(9600);
    CircuitPlayground.begin();
    pulseSensor.analogInput(PulseSensorPin);
    pulseSensor.setThreshold(THRESHOLD);  // Set pulse sensor threshold
    pulseSensor.begin();
    countDiff = 0;
}

void loop() {
    if (pulseSensor.sawStartOfBeat())
    {
        raw_IBI[index % s] = pulseSensor.getInterBeatIntervalMs();
        index++;


        if (index >= s) { // Compute SMA after enough readings

            IBI = computeSMA(raw_IBI, s);

            ibi[countDiff % SIZE] = IBI;

            Serial.print("IBI: ");

            Serial.println(IBI);

            if (lastIBI > 0) {
                int IBI_diff = IBI - lastIBI;
                IBIDiffs[countDiff % SIZE] = IBI_diff; // Store the difference
                countDiff++;

                // Compute RMSSD and SDNN after collecting enough differences
                if (countDiff >= SIZE) {
                    RMSSD = computeRMSSD(IBIDiffs, SIZE);
                    sdnn = calculateSDNN(ibi, SIZE);

                    Serial.print("RMSSD: ");

                    Serial.println(RMSSD);
                    Serial.print("SDNN: ");

                    Serial.println(sdnn);

                    // Check stress detection
                    stressAlert.checkStressDetection(RMSSD, sdnn);
                }
            }

            // Update last IBI
            lastIBI = IBI;
        }
    }


    delay(20); // Stabilize the readings
}
