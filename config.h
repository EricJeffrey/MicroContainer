#if !defined(CONFIG_H)
#define CONFIG_H

#include <string>

using std::string;

static const string ImageDirPath = "./build/images/";
static const string ImageRepoFileName = "repo.json";
static const string LayerDirPath = "./build/layers/";
static const string OverlayDirPath = "./build/overlay/";
static const string ContainerDirPath = "./build/containers/";
static const string ContainerRepoFileName = "containerRepo.json";

static const string ContainerRtPath = "/usr/bin/crun";
static const string ContainerRtName = "crun";

// registry address
static const string DefaultRegAddr = "https://ejlzjv4p.mirror.aliyuncs.com";

// static const string DefaultRegAddr = "https://docker.mirrors.ustc.edu.cn";

// read timeout when pulling image from registry
static const time_t ReadTimeoutInSec = 30;

static const int FetchBlobMaxRetryTimes = 15;

#endif // CONFIG_H
