#pragma once

#include "ofMain.h"
#define USE_KINECT_2 1
#define USE_Memory 1


#if USE_KINECT_2
#include "ofxMultiKinectV2.h"
#else
#include "ofxKinect.h"
#endif
#include "ofxOpenCv.h"
#include "ofxColorMap.h"
#include "ofxFboBlur.h"
#include "ofxFaceTrackerThreaded.h"
#include "ofxCv.h"
#include "FaceTrackerThreaded.h"
#include "GpuRegistration.h"
#if USE_Memory
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <fstream>
#endif


class Timer {
  uint64_t startTime;
  bool isPlaying;
public:
  Timer() {
    startTime = 0;
    isPlaying = false;
  }

  uint64_t getStartTime() {
    return startTime;
  }

  void start() {
    if (!isPlaying) {
      isPlaying = true;
      startTime = ofGetElapsedTimeMillis();
    }
  }

  bool isStarted() {
    return isPlaying;
  }

  uint64_t getElapsedMillis() {
    if (isPlaying) {
      return ofGetElapsedTimeMillis() - startTime;
    } else {
      return 0;
    }
  }

  void stop() {
    startTime = 0;
    isPlaying = false;
  }
    
  void reset() {
    startTime = ofGetElapsedTimeMillis();
  }
};


static string depthVertexShader =
  STRINGIFY(
	    varying vec2 texCoordVarying;
	    void main() {
	      texCoordVarying = gl_MultiTexCoord0.xy;
	      gl_Position = ftransform();
	    }
	    );

static string depthFragmentShader =
STRINGIFY(
    uniform sampler2DRect tex;
    void main()
    {
      vec4 col = texture2DRect(tex, gl_TexCoord[0].xy);
        float value = col.r;
        float low1 = 650.0;
        float high1 = 950.0;
        float low2 = 1.0;
        float high2 = 0.0;
        float d = clamp(low2 + (value - low1) * (high2 - low2) / (high1 - low1), 0.0, 1.0);
        if (d == 1.0) {
            d = 0.0;
        }
      gl_FragColor = vec4(vec3(d), 1.0);
    }
);

static string irFragmentShader =
  STRINGIFY(
	    uniform sampler2DRect tex;
	    void main()
	    {

              vec4 col = texture2DRect(tex, gl_TexCoord[0].xy);
              float value = col.r / 65535.0;
              gl_FragColor = vec4(vec3(value), 1.0);
	    }
	    );


constexpr int win_width = 512 * 2;
constexpr int win_height = 424 * 2;
constexpr int win_gray_width_kinect = 512;
constexpr int win_gray_height_kinect = 424;

class ofSoundBufferCereal : public ofSoundBuffer {
public:
#if USE_Memory
  friend class cereal::access;
#endif
  template<typename Archive>
    void serialize(Archive& archive) {
    archive( buffer, channels, samplerate, tickCount, soundStreamDeviceID);
  }
};


class FaceAnimation {
 public:
  ofSoundBufferCereal soundBuffer;
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
    soundStream.stop();
  }
  void tranposeRotation(ofMatrix4x4 *_Matrix);
    
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

  ofxColorMap         colormap;
  ofImage             imageColor,imageGray;
    
  ofxCv::ContourFinder   contourFinder;
  FaceTrackerThreaded trackerFace;
    
  vector<FaceAnimation>  faceAnimationVect;
  vector<FaceAnimation>  finalAnimations;
  FaceAnimation          *faceAnimationPtr;
  ofMesh             animation, bufferAnimation, animationMouth;
  bool                   rec = false,play = false;

  /*
   *
   * ATTENTION: il ne faut pas que d'autres logiciels utilisent du son
   * genre firefox etc
   * sinon ca casse tout
   * ou alors faut faire des reglages chelou avec alsa ou jack, et flemme
   *
   */
  ofSoundStream          soundStream;
  int                    bufferCounter = 0;

  mutex audioMutex;

  ofImage imgGr;
  ofPixels tmpPixels;

  ofFbo fboGray;
  ofImage grayFboImage;

  ofFbo fboColorMaskAndBackground;

  int numFiles;


  ofxFboBlur blur;

  Timer incomingPerson;
  Timer exitPerson;

  int randomAnimation = 0;

  ofImage temp;
  ofImage saveImage;

  bool timerReset;

  float rotation;
  ofVec2f position = {0, 0};
  ofVec2f scale = {1, 1};

  
};
