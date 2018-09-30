import numpy as np
import cv2
import time

def next_frame(capture):
    ret, frame = capture.read()
    image = cv2.flip(frame, 0) #cv2.cvtColor(frame, cv2.COLOR
    image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    return image

cap = cv2.VideoCapture(0)
sum_img = (next_frame(cap)*0).astype('float')
frame_count = 1

while(True):
    
    # Capture frame-by-frame
    img = next_frame(cap) 

    rows,cols = img.shape

    pts1 = np.float32([[50,50],[200,50],[50,200]])
    pts2 = np.float32([[10,100],[200,50],[100,250]])

    M = cv2.getAffineTransform(pts1,pts2)
 
    dst = cv2.warpAffine(img,M,(cols,rows))    
    portion = 1.0/frame_count
    norm = 25.0*1.0/256.0
    sum_img = cv2.addWeighted( img.astype('float'), norm*portion, sum_img, 1.0-portion, 0)
    # Display the resulting frame
    cv2.imshow('frame', sum_img)#cv2.cvtColor(sum_img.astype(np.uint8), cv2.COLOR_BGR2GRAY))
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
    frame_count += 1

# When everything done, release the capture
cap.release()
cv2.destroyAllWindows()
