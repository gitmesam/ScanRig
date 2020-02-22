import numpy as np
import threading, logging, cv2


class CaptureDevice(object):
    def __init__(self, indexCam, framesToSave):
        self.indexCam = indexCam
        (self.status, self.frame) = (None, None)
        self.nbSavedFrame = 0
        self.framesToSave = framesToSave

        # Initialize camera
        self.capture = cv2.VideoCapture()

        v = self.capture.open(self.indexCam, apiPreference=cv2.CAP_V4L2)
        if v:
            self.fillBuffer()
            self.retrieveFrame()
        else:
            logging.warning("Skip invalid stream ID {}".format(self.indexCam))
            self.stop()

    def fillBuffer(self):
        for i in range(int(self.capture.get(cv2.CAP_PROP_BUFFERSIZE)) + 1):
            self.grabFrame()

    def grabFrame(self):
        # print("image grabbed")
        ret = self.capture.grab()
        if not ret:
            logging.warning("Image cannot be grabbed")

    def retrieveFrame(self):
        self.status, self.frame = self.capture.retrieve()
        if not self.status:
            logging.warning("Image cannot be retrieved")

    def saveFrame(self):
        self.framesToSave.put((self.indexCam, self.nbSavedFrame, self.frame))
        self.nbSavedFrame += 1

    def stop(self):
        self.capture.release()
        if not self.capture.isOpened():
            print(f"Device n°{self.indexCam} closed")