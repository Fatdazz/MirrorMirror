#pragma once

#include "ofMain.h"
#define USE_KINECT_2 1

#if USE_KINECT_2
#include "ofxMultiKinectV2.h"
#else
#include "ofxKinect.h"
#endif
#include "ofxOpenCv.h"
#include "ofxColorMap.h"
#include "ofxMultiFboBlur.h"
#include "ofxFaceTrackerThreaded.h"
#include "ofxCv.h"
#include "FaceTrackerThreaded.h"

#include "GpuRegistration.h"


class FaceAnimation {
 public:
  ofSoundBuffer soundBuffer;
  vector<ofMesh>  face;
};

class ofApp : public ofBaseApp{

 public:
  void setup();
  void update();
  void draw();

  void keyPressed(int key);
  void keyReleased(int key);
  void mouseMoved(int x, int y );
  void mouseDragged(int x, int y, int button);
  void mousePressed(int x, int y, int button);
  void mouseReleased(int x, int y, int button);
  void mouseEntered(int x, int y);
  void mouseExited(int x, int y);
  void windowResized(int w, int h);
  void dragEvent(ofDragInfo dragInfo);
  void gotMessage(ofMessage msg);
  void audioIn(ofSoundBuffer& buffer);
  void audioOut(ofSoundBuffer &outBuffer);
  void exit() {
    trackerFace.waitForThread();
    kinect.close();
  }
  void tranposeRotation(ofMatrix4x4 *_Matrix);
  vector<int> consecutive(int start, int end);

    
  bool debug = true;


#if USE_KINECT_2
  ofxMultiKinectV2 kinect;
  ofShader depthShader;
  ofShader irShader;
  GpuRegistration gr;
  ofTexture colorTex0;
  ofTexture depthTex0;

#else
  ofxKinect kinect;
#endif
    
  int nearThreshold;
  int farThreshold;
    
  ofxCvGrayscaleImage grayImage;
  ofxCvGrayscaleImage grayThreshNear;
  ofxCvGrayscaleImage grayThreshFar;
  ofxCvGrayscaleImage grayImageMap;
    
  ofxColorMap         colormap;
  ofImage             imageColor,imageGray;
  ofFbo               imageFbo;
  ofxMultiFboBlur     imageBlur;
    
  ofxCv::ContourFinder   contourFinder;
  FaceTrackerThreaded trackerFace;
    
  vector<FaceAnimation>  faceAnimationVect;
  FaceAnimation          *faceAnimationPtr;
  ofMesh             Animation, bufferAnimation;
  bool                   rec,play;

  /*
   * ATTENTION: il ne faut pas que d'autres logiciels utilisent du son
   * genre firefox etc
   * sinon ca casse tout
   * ou alors faut faire des reglages chelou avec alsa ou jack, et flemme
   *
   */
  ofSoundStream          soundStream;
  int                    bufferCounter;

  mutex audioMutex;

  ofImage tmp;

      ofImage img;


      bool trucAlexTropCool = false;
};
