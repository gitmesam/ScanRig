import numpy as np
import threading, logging, cv2

import config, cameraSettings

class GetFrameThread(threading.Thread):
    def __init__(self, capDevice):
        threading.Thread.__init__(self)
        self.capDevice = capDevice

    def run(self):
        (self.capDevice.status, self.capDevice.frame) = self.capDevice.capture.read()
        self.capDevice.frame = cv2.cvtColor(self.capDevice.frame, cv2.COLOR_YUV2BGR_UYVY) # To use only with the FSCAM_CU135
        print(f"Reading frame from cam {self.capDevice.indexCam}")


class CaptureDevice(object):
    def __init__(self, indexCam, framesToSave):
        self.indexCam = indexCam
        (self.status, self.frame) = (None, None)
        self.nbSavedFrame = 0
        self.framesToSave = framesToSave
        self.settings = cameraSettings.cameraSettingsList

        # Initialize camera
        self.capture = cv2.VideoCapture()

        v = self.capture.open(self.indexCam, apiPreference=cv2.CAP_V4L2)
        if v:
            for i in range(int(self.capture.get(cv2.CAP_PROP_BUFFERSIZE)) + 1):
                self.grabFrame() # Needed to make it work well
            cameraSettings.setAttributes(self.capture)
            cameraSettings.getAttributes(self.capture)
        else:
            logging.warning("Skip invalid stream ID {}".format(self.indexCam))
            self.stop()

    def grabFrame(self):
        if not self.capture.grab():
            logging.warning("Image cannot be grabbed")

    def retrieveFrame(self):
        self.status, self.frame = self.capture.retrieve()
        # self.frame = cv2.cvtColor(self.frame, cv2.COLOR_YUV2BGR_UYVY) # To use only with the FSCAM_CU135
        if not self.status:
            logging.warning("Image cannot be retrieved")

    def saveFrame(self):
        self.framesToSave.put((self.indexCam, self.nbSavedFrame, self.frame))
        self.nbSavedFrame += 1

    def stop(self):
        self.capture.release()
        print("Closing capture")
        if not self.capture.isOpened():
            print("Capture closed")