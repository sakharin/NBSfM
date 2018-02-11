#ifndef NBSfM_hpp
#define NBSfM_hpp

#include <iostream>
#include "opencv2/opencv.hpp"
#include <unistd.h>

using namespace std;
using namespace cv;

#include <sys/types.h>
#include <dirent.h>
typedef std::vector<std::string> StringVec;

class NBSfM {
 private:
  // Parameters ====================
  // Input
  string workspace_path_;
  string image_folder_path_;
  int max_num_frames_;
  vector< string > image_paths_;
  int num_images_;
  // Parameters ====================

  // Data ==========================
  // Images
  vector< Mat > images_;
  int image_width_;
  int image_height_;
  // Data ==========================

  // Parameter functions  ==========
  bool CheckParameters(int argc, char * argv[]);
  void ShowParameters();
  bool CheckWorkspace();
  bool CheckImageFolder();
  bool CheckImage(string image_name);
  bool CheckImagesInFolder();
  void Help(int argc, char *argv[]);
  // Parameter functions  ==========

  // Helper functions ==============
  inline bool EndsWith(std::string const & value, std::string const & ending);
  void ReadDirectory(const std::string& name, StringVec& v);
  // Helper functions ==============

  // 3D reconstruction functions ===
  bool LoadImages();
  // 3D reconstruction functions ===

 public:
  NBSfM(int argc, char * argv[]);
};
#endif /* NBSfM_hpp */
