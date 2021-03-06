import numpy as np
import cv2

from PySide2 import QtWidgets
from PySide2.QtCore import QSize, QObject, Slot, Property
from PySide2.QtQuick import QQuickImageProvider
from PySide2.QtGui import QImage, QPixmap

class ImageProvider(QQuickImageProvider):
    def __init__(self, devices):
        super(ImageProvider, self).__init__(QQuickImageProvider.Pixmap)
        self.previewDevices = devices

    def requestPixmap(self, id, size, requestedSize):
        camId = int(id.split("/")[0])
        if camId == -1: 
            return
        if self.previewDevices.isEmpty():
            return

        frame = self.previewDevices.getDevice(0).frame # Because we only have one device
        qImg = QImage(frame, frame.shape[1], frame.shape[0], QImage.Format_RGB888).rgbSwapped()
        return QPixmap.fromImage(qImg)