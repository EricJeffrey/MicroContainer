#if !defined(CONFIG_H)
#define CONFIG_H

#include <string>

using std::string;

static const string imageDirPath = "./build/images/";
static const string imageRepoFileName = "repo.json";
static const string layerDirPath = "./build/layers/";
static const string overlayDirPath = "./build/overlay/";
static const string containerDirPath = "./build/containers/";
static const string containerRepoFileName = "containerRepo.json";

static const string containerRtPath = "/usr/bin/crun";
static const string containerRtName = "crun";

// registry address
static const string defaultRegAddr = "https://ejlzjv4p.mirror.aliyuncs.com";

// static const string defaultRegAddr = "https://docker.mirrors.ustc.edu.cn";

// read timeout when pulling image from registry
static const time_t readTimeoutInSec = 30;

#endif // CONFIG_H
