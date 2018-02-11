#include "NBSfM.hpp"

NBSfM::NBSfM(int argc, char *argv[]) :
    workspace_path_("."),
    image_folder_path_("./images"),
    video_path_(""),
    max_num_frames_(30),
    is_use_images_(false),
    is_use_video_(false) {
  if (!CheckParameters(argc, argv)) {
    exit(-1);
  }

  ShowParameters();

  if (is_use_video_) {
    ExportVideoFrames();
  }
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
    } else if (index + 1 < argc && strcmp(argv[index], "--image_paths") == 0) {
      num_images_ = 0;
      image_paths_.clear();

      index += 1;
      while (index < argc && string(argv[index]).compare(0, 2, "--") != 0) {
        if (access(argv[index], F_OK) != 0) {
          cerr << "Cannot read an image : " << argv[index] << "." << endl;
          return false;
        }
        image_paths_.push_back(argv[index]);
        if (!CheckImage(image_paths_.back())) {
          cerr << "Cannot read an image : " << argv[index] << "." << endl;
          return false;
        }
        num_images_++;
        index += 1;
      }
      is_use_video_ = false;
      is_use_images_ = true;
    } else if (index + 1 < argc && strcmp(argv[index], "--image_folder") == 0) {
      image_folder_path_.assign(argv[index + 1]);
      if (!CheckImageFolder()) {
        cerr << "Cannot find folder : " << image_folder_path_ << "." << endl;
        return false;
      } else {
        CheckImagesInFolder();
      }
      is_use_video_ = false;
      is_use_images_ = true;
      index += 2;
    } else if (index + 3 < argc && strcmp(argv[index], "--video") == 0 &&
               string(argv[index + 1]).compare(0, 2, "--") != 0 &&
               string(argv[index + 2]).compare(0, 2, "--") != 0 &&
               string(argv[index + 3]).compare(0, 2, "--") != 0) {
      video_path_.assign(argv[index + 1]);
      max_num_frames_ = strtod(argv[index + 2], NULL);
      image_folder_path_.assign(argv[index + 3]);
      if (max_num_frames_ < 2) {
        cerr << "max_num_frames must be greater than 2." << endl;
        return false;
      }
      CheckVideo();
      is_use_images_ = false;
      is_use_video_ = true;
      index += 4;
    } else if (index + 2 < argc && strcmp(argv[index], "--video") == 0 &&
               string(argv[index + 1]).compare(0, 2, "--") != 0 &&
               string(argv[index + 2]).compare(0, 2, "--") != 0) {
      video_path_.assign(argv[index + 1]);
      max_num_frames_ = strtod(argv[index + 2], NULL);
      image_folder_path_.assign(workspace_path_ + "/images");
      if (max_num_frames_ < 2) {
        cerr << "max_num_frames must be greater than 2." << endl;
        return false;
      }
      CheckVideo();
      is_use_images_ = false;
      is_use_video_ = true;
      index += 3;
    } else {
      Help(argc, argv);
      return false;
    }
  }

  if (access(workspace_path_.c_str(), W_OK) != 0) {
    cerr << "Cannot write on workspace_path : " << workspace_path_ << "." << endl;
    return false;
  }
  if (is_use_images_) {
    if (image_paths_.size() < 2) {
        cerr << "At least 2 images are required." << endl;
        return false;
    }
  } else if (is_use_video_) {
    if (access(video_path_.c_str(), F_OK) != 0) {
      cerr << "Cannot find video_path : " << video_path_ << "." << endl;
      return false;
    }
    if (access(image_folder_path_.c_str(), F_OK | W_OK) != 0) {
      cerr << "Cannot write on image_folder_path : " << image_folder_path_ << "." << endl;
      return false;
    }
  } else {
    cerr << "A video or 2 images are required." << endl;
    return false;
  }
  return true;
}

void NBSfM::ShowParameters() {
  cout << "                      Parameters" << endl;
  cout << "                         Input" << endl;
  cout << "            workspace_path : " << workspace_path_ << endl;

  if (is_use_images_) {
    vector< string >::iterator it = image_paths_.begin();
  cout << "                num_images : " << num_images_ << endl;
  cout << "               image_paths : " << *it << endl;
  ++it;
    for (; it < image_paths_.end(); ++it) {
  cout << "                             " << *it << endl;
    }
  } else if (is_use_video_) {
  cout << "                video_path : " << video_path_ << endl;
  cout << "         image_folder_path : " << image_folder_path_ << endl;
  cout << "            max_num_frames : " << max_num_frames_ << endl;
  }
}

bool NBSfM::CheckWorkspace() {
  // Check images
  bool res;
  image_folder_path_.assign(workspace_path_ + "/images");
  res = CheckImageFolder();
  if (res) {
    CheckImagesInFolder();
    is_use_video_ = false;
    is_use_images_ = true;
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

bool NBSfM::CheckVideo() {
  if (access(video_path_.c_str(), F_OK | R_OK) != 0) {
    return false;
  }
  if (access(image_folder_path_.c_str(), F_OK) != 0) {
    if (!MakeDir(image_folder_path_)) {
      return false;
    }
  } else if (access(image_folder_path_.c_str(), F_OK | W_OK) != 0) {
    return false;
  }
  return true;
}

void NBSfM::Help(int argc, char *argv[]) {
  cout << "Usage : " << argv[0] << " [Parameters]" << endl;
  cout << "Parameters : (a duplicate parameter overrides previous one)" << endl;
  cout << "  Input" << endl;
  cout << "    [--workspace_path workspace_path]" << endl;
  cout << "    [--image_paths image1_path image2_path [image3_path ...]]" << endl;
  cout << "    [--image_folder image_folder]" << endl;
  cout << "    [--video video_path image_folder_path [max_num_frames]]" << endl;
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

bool NBSfM::MakeDir(string path) {
  string mkdir_cmd = "mkdir -p " + path;
  const int dir_err = system(mkdir_cmd.c_str());
  if (-1 == dir_err) {
    cerr << "Error creating directory!n";
    return false;
  }
  return true;
}

bool NBSfM::ExportVideoFrames() {
  cout << endl << endl << "Export frames from a video.." << endl;

  // Clear image paths
  num_images_ = 0;
  image_paths_.clear();

  // Get video
  VideoCapture cap(video_path_.c_str());
  if(!cap.isOpened()) {
    cout << "  Cannot read video." << endl;
    return false;
  }
  int num_frames = cap.get(CV_CAP_PROP_FRAME_COUNT);
  num_frames = min(num_frames, max_num_frames_);
  for (int i = 0; i < num_frames; i++) {
    cout << "  Exporting image " << (i + 1) << "/" << num_frames << endl;

    // Get a new frame from camera
    Mat frame;
    cap >> frame;

    // Write frames
    std::ostringstream ss;
    ss << image_folder_path_ << "/" << std::setw(4) << std::setfill('0') << i << ".png";
    string frame_path(ss.str());
    imwrite(frame_path, frame);
    image_paths_.push_back(ss.str());
    num_images_++;
  }
  return true;
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
