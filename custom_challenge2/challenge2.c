#pragma config(StandardModel, "EV3_REMBOT")

// global variables
int nTurnRatio = 0;
int power = 0;
int distance = 0;
float degrees = 0;

// calibration
int obstacle = 13;
int rotatepower = 16;
int movepower = 50;
int moving_time = 1000;
int waiting_time = 500;

// problem's given data
int dead_end_wall = 122;    // 4 feet
float opposite_wall = 30.5; // 1 feet

// possible states
enum states {
    IDLE  = 0, // IDLE
    FWD   = 1, // FORWARD
    LEFT  = 2, // TURN LEFT
    RIGHT = 3, // TURN RIGHT
    CWROT  = 4, // CLOCKWISE ROTATION (90°)
    CCWROT = 5, // COUNTERCLOCKWISE ROTATION (90°)
    UTURN2NEXTTRACK = 6, // ROTATE TO NEXT TRACK (180°)
    AVOIDOBSTACLE   = 7  // AVOID AN OBSTACLE
};
states state = IDLE;

// proportional turn variables
float threshold = 70;
float color = 0;
float error = 0;
float k = 1;

task colorReflected() {
    while (1) {
        color = getColorReflected(S3);
        error = color - threshold;
        sleep(10);
    }
}

task motorTask() {
    while (1) {
        setMotorSync(leftMotor, rightMotor, nTurnRatio, power);
        sleep(10);
    }
}

task DisplayTask() {
    while (1) {
        displayTextLine(2, "ColorReflected: %d", color);
        displayTextLine(3, "state: %d", state);
        displayTextLine(4, "power: %d", power);
        displayTextLine(5, "degrees: %d", degrees);
        displayTextLine(6, "k: %f", k);
        displayTextLine(7, "distance: %f", distance);
        displayTextLine(8, "degrees: %d", degrees);
        sleep(10); // 100 Hz
    }
}

task DistanceTask() {
    while (1) {
        distance = getUSDistance(S4);
        sleep(10);
    }
}

task GyroTask() {
    while (1) {
        degrees = getGyroDegrees(S2);
        sleep(10);
    }
}

task main() {
    startTask(colorReflected);
    startTask(motorTask);
    startTask(DistanceTask);
    startTask(GyroTask);
    clearDebugStream();
    startTask(DisplayTask);

    while (1) {
        //=====================================================================
        //   1st SWITCH (checking states)
        switch (state) {
            case IDLE:
                // 4ft / checking if it is the dead-end wall.
                if (distance == dead_end_wall) {
                    state = CWROT;
                    break;
                }

                state = FWD;
                break;

            case FWD: // FORWARD
                if (error < 0) {
                    state = LEFT;
                    break;
                }
                if (error > 0) {
                    state = RIGHT;
                    break;
                }
                break;

            case LEFT: // TURN LEFT
                if (distance <= obstacle) { // obstacle detected
                    state = AVOIDOBSTACLE;
                    break;
                }
                if (error > 0) {
                    state = RIGHT;
                    break;
                }
                if (distance == dead_end_wall) { // 4ft / possible dead-end wall.
                    state = IDLE;
                    break;
                }
                break;

            case RIGHT: // TURN RIGHT
                if (distance <= obstacle) { // obstacle detected
                    state = AVOIDOBSTACLE;
                    break;
                }
                if (error < 0) {
                    state = LEFT;
                    break;
                }
                if (distance == dead_end_wall) { // 4ft / possible dead-end wall.
                    state = IDLE;
                    break;
                }
                break;

            case CWROT: // CLOCKWISE ROTATION (90°)
                // 1ft / checking if there is a wall on the opposite side of the next track
                if (distance <= opposite_wall && distance >= obstacle) {
                    state = UTURN2NEXTTRACK; // turn to the next track
                    break;
                }

                state = CCWROT;
                break;

            case CCWROT: // COUNTERCLOCKWISE ROTATION (90°)
                if (error < 0) {
                    state = LEFT;
                    break;
                }
                if (error > 0) {
                    state = RIGHT;
                    break;
                }
                break;

            case UTURN2NEXTTRACK: // ROTATE TO NEXT TRACK (180°)
                state = IDLE;
                break;

            case AVOIDOBSTACLE: // AVOID AN OBSTACLE
                if (error < 0) {
                    state = LEFT;
                    break;
                }
                if (error > 0) {
                    state = RIGHT;
                    break;
                }
        }

        //=====================================================================
        //  2nd SWITCH (running states)
        switch (state) {
            case IDLE:
                nTurnRatio = 0;
                power = 0;
                break;

            case FWD: // FORWARD
                nTurnRatio = 0;
                power = movepower;
                sleep(moving_time);
                break;

            case LEFT: // TURN LEFT
                if (error <= -20) {
                    k = 5;
                }
                else if (error > -20 && error < -10) {
                    k = 3;
                }
                else {
                    k = 1;
                }

                nTurnRatio = k * error;
                power = movepower;
                break;

            case RIGHT: // TURN RIGHT
                if (error >= 20) {
                    k = 0.4;
                }
                else if (error < 20 && error > 10) {
                    k = 2;
                }
                else {
                    k = 1;
                }

                nTurnRatio = k * error;
                power = movepower;
                break;

            case CWROT: // CLOCKWISE ROTATION (90°)
                nTurnRatio = 0;
                power = 0;
                sleep(waiting_time);
                resetGyro(S2);
                sleep(waiting_time);

                while (degrees <= 85) {
                    nTurnRatio = 100;
                    power = rotatepower;
                }
                break;

            case CCWROT: // COUNTERCLOCKWISE ROTATION (90°)
                nTurnRatio = 0;
                power = 0;
                sleep(waiting_time);
                resetGyro(S2);
                sleep(500);

                while (degrees >= -87) {
                    nTurnRatio = -100;
                    power = rotatepower;
                }

                nTurnRatio = 0;
                power = movepower;
                sleep(moving_time);
                break;

            case UTURN2NEXTTRACK: // ROTATE TO NEXT TRACK (180°)
                nTurnRatio = 0;
                power = 0;
                sleep(waiting_time);
                resetGyro(S2);
                sleep(waiting_time);
                while (degrees <= 180) {
                    nTurnRatio = 100;
                    power = rotatepower;
                }
                nTurnRatio = 0;
                power = movepower;
                sleep(moving_time);
                dead_end_wall = 0; // disable the checking for the dead-end wall
                opposite_wall = 0;
                break;

            case AVOIDOBSTACLE: // AVOID AN OBSTACLE
                nTurnRatio = 0;
                power = 0;
                sleep(waiting_time);
                resetGyro(S2);
                sleep(waiting_time);

                while (degrees >= -85) {
                    nTurnRatio = -100;
                    power = rotatepower;
                }

                nTurnRatio = 0;
                power = movepower;
                sleep(moving_time);

                nTurnRatio = 0;
                power = 0;
                sleep(waiting_time);
                resetGyro(S2);
                sleep(waiting_time);

                while (degrees <= 85) {
                    nTurnRatio = 100;
                    power = rotatepower;
                }

                if (distance > obstacle) {
                    nTurnRatio = 0;
                    power = movepower;
                    sleep(moving_time);
                }

                break;
        }
    }
}
