#include "NBSfM.hpp"

NBSfM::NBSfM(int argc, char *argv[]) :
    workspace_path_("."),
    image_folder_path_("./images"),
    max_num_frames_(30) {
  if (!CheckParameters(argc, argv)) {
    exit(-1);
  }

  ShowParameters();

  LoadImages();
}

bool NBSfM::CheckParameters(int argc, char * argv[]) {
  int index = 1;
  while (index < argc) {
    if (index + 1 < argc && strcmp(argv[index], "--workspace_path") == 0) {
      workspace_path_.assign(argv[index + 1]);
      if (!CheckWorkspace()) {
        return false;
      }
      index += 2;
    } else {
      Help(argc, argv);
      return false;
    }
  }
  if (image_paths_.size() < 2) {
      cerr << "At least 2 images are required." << endl;
      return false;
  }
  return true;
}

void NBSfM::ShowParameters() {
  cout << "                      Parameters" << endl;
  cout << "                         Input" << endl;
  cout << "            workspace_path : " << workspace_path_ << endl;

  vector< string >::iterator it = image_paths_.begin();
  cout << "                num_images : " << num_images_ << endl;
  cout << "               image_paths : " << *it << endl;
  ++it;
  for (; it < image_paths_.end(); ++it) {
  cout << "                             " << *it << endl;
  }
}

bool NBSfM::CheckWorkspace() {
  // Check images
  bool res;
  image_folder_path_ = workspace_path_ + "/images";
  res = CheckImageFolder();
  if (res) {
    CheckImagesInFolder();
  } else {
    num_images_ = 0;
    image_paths_.clear();
  }
  return true;
}

bool NBSfM::CheckImageFolder() {
  if (access(image_folder_path_.c_str(), F_OK | R_OK) == 0) {
    return true;
  }
  return false;
}

bool NBSfM::CheckImage(string image_name) {
  return (EndsWith(image_name, ".JPG") ||
          EndsWith(image_name, ".JPEG") ||
          EndsWith(image_name, ".jpeg") ||
          EndsWith(image_name, ".jpg") ||
          EndsWith(image_name, ".PNG") ||
          EndsWith(image_name, ".png"));
}

bool NBSfM::CheckImagesInFolder() {
  num_images_ = 0;
  image_paths_.clear();

  StringVec str_vec;
  ReadDirectory(image_folder_path_, str_vec);
  sort(str_vec.begin(), str_vec.end());
  vector< string > tmp_image_paths;
  for (unsigned int i = 0; i < str_vec.size(); i++) {
    if (CheckImage(str_vec[i])) {
      tmp_image_paths.push_back(image_folder_path_ + "/" + str_vec[i]);
    }
  }
  if (tmp_image_paths.size() >= 2) {
    image_paths_.clear();
    for (unsigned int i = 0; i < tmp_image_paths.size(); i++) {
      image_paths_.push_back(tmp_image_paths[i]);
    }
    num_images_ = image_paths_.size();
  } else {
    return false;
  }
  return true;
}

void NBSfM::Help(int argc, char *argv[]) {
  cout << "Usage : " << argv[0] << " [Parameters]" << endl;
  cout << "Parameters : (a duplicate parameter overrides previous one)" << endl;
  cout << "  Input" << endl;
  cout << "    [--workspace_path workspace_path]" << endl;
}

inline bool NBSfM::EndsWith(std::string const & value, std::string const & ending) {
  // https://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void NBSfM::ReadDirectory(const std::string& name, StringVec& v) {
  DIR* dirp = opendir(name.c_str());
  struct dirent * dp;
  while ((dp = readdir(dirp)) != NULL) {
    v.push_back(dp->d_name);
  }
  closedir(dirp);
}

bool NBSfM::LoadImages() {
  cout << endl << endl << "Load Images.." << endl;
  int img_idx = 0;
  for (auto image_path : image_paths_) {
    cv::Mat img = cv::imread(image_path, CV_LOAD_IMAGE_COLOR);
    if (!img.data) {
      throw runtime_error("Could not open image.");
    }
    if (img_idx == 0) {
      image_width_ = img.cols;
      image_height_ = img.rows;
    } else {
      if (image_width_ != img.cols || image_height_ != img.rows) {
        throw runtime_error("An image doesn't have the same size.");
      }
    }
    images_.push_back(img);
    cout << "  " << img_idx + 1 << " / " << num_images_ << endl;
    img_idx++;
  }
  return true;
}

int main(int argc, char * argv[]) {
  NBSfM(argc, argv);
  return 0;
}
