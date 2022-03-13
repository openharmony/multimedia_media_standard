# Media<a name="EN-US_TOPIC_0000001147574647"></a>

-   [Introduction](#section1158716411637)
-   [Directory Structure](#section161941989596)
-   [Repositories Involved](#section1533973044317)

## Introduction<a name="section1158716411637"></a>

The  **media\_standard**  repository provides a set of simple and easy-to-use APIs for you to access the system and media resources.

It offers various media services covering audio, videos, and media storage. The following media capabilities are provided:

-   Audio playback and recording
-   Video playback and recording

**Figure  1**  Position in the subsystem architecture<a name="fig99659301300"></a>  


![](figures/en-us_image_0000001105973932.png)

## Directory Structure<a name="section161941989596"></a>

The structure of the repository directory is as follows:
```
/foundation/multimedia/media_standard
├── frameworks                           # Framework code
│   ├── innerkitsimpl                    # Native framework implementation
│   ├── kitsimpl                         # JS framework implementation
│   ├── videodisplaymanager              # Video display implementation
├── interfaces                           # External APIs
│   ├── innerkits                        # Native external interface files
│   └── kits                             # External JS API files
├── services                             # Service implementation
│   ├── include                          # External header files of services
│   ├── services                         # Service framework
│   │   ├── player                       # Player C/S framework
│   │   ├── recorder                     # Recorder C/S framework
│   │   ├── avcodec                      # Codec C/S framework
│   │   ├── avcodeclist                  # Codec capability C/S framework
│   │   ├── avmetadatahelper             # Metadata C/S framework
│   │   ├── media_data_source            # Datasource C/S framework
│   │   ├── sa_media                     # Media master process C/S framework
│   │   ├── factory                      # Engine factory
│   │   ├── engine_intf                  # Engine factory API
│   ├── engine                           # Engine implementation
│   │   ├── gstreamer                    # Gstreamer engine
│   │   │   ├── player                   # Player engine
│   │   │   ├── recorder                 # Recorder engine
│   │   │   ├── avcodec                  # Codec engine
│   │   │   ├── avcodeclist              # Codec capability engine
│   │   │   ├── avmetadatahelper         # Metadata engine
│   │   │   ├── plugins                  # Engine self-developed plugins
│   │   │   ├── common                   # Engine General Resources
│   │   │   ├── factory                  # Engine factory implementation
│   │   │   ├── loader                   # Engine resources loading
│   ├── utils                            # Basic resources of the subsystem
├── LICENSE                              # License file
└── ohos.build                           # Build file
```

## Repositories Involved<a name="section1533973044317"></a>

Media repository: multimedia\media_standard