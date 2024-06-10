/**
 * @file UT_ConlesEventDemoLiCam.cxx
 * @brief Demo for ConlesEvent of LiveInternetCamera(a.k.a LiCam).
 *  1) LiCam has a network service module which used to accept the connection from the client.
 *      a) Client will receive the camera's live audio&video stream by defult.
 *           |-> Client may select audio-only, video-only, or both after connected.
 *      b) Only VIP client may continuly HiRes audio&video stream, others will receive LoRes audio&video stream.
 *      c) One of VIP client may request bidirectional audio&video communication.
 *  2) LiCam has video/audio capture module, video/audio encode module, stream mux module.
 *      a) video capture in HiRes by default, use video resize module to get LoRes video.
 */

/**
 * ModMgrObj: all modules in LiCam is managed by ModMgrObj,
 *      which means created/destroyed/started/stopped by ModMgrObj,
 *      which also means all modules MUST post ModuleAliveEvent to ModMgrObj in 1s interval.
 * CliObjFactory: ClientObject Factory, used to create ClientObject.
 * CliObj: ClientObject, created by CliObjFactory, used to simulate the client behaviors.
 * SrvObj: ServerObject, used to simulate the server behaviors.
 * HiResStrmMuxObj: High Resolution Stream Object, used to simulate the HiRes stream.
 * LoResStrmMuxObj: Low Resolution Stream Object, used to simulate the LoRes stream.
 * HiResVidEncObj: High Resolution Video Encode Object, used to simulate the HiRes video encode.
 * LoResVidEncObj: Low Resolution Video Encode Object, used to simulate the LoRes video encode.
 * VidResizeObj: Video Resize Object, used to simulate the video resize.
 * AudEncObj: Audio Encode Object, used to simulate the audio encode.
 * VidCapObj: Video Capture Object, used to simulate the video capture.
 * AudCapObj: Audio Capture Object, used to simulate the audio capture.
 *
 */

/**
 * @details ModMgrObj
 *    subEVT: ModuleAliveEvent, ModuleCreateEvent, ModuleDestroyEvent
 *    postEVT: ModuleStartEvent, ModuleStopEvent
 */

/**
 * @details CliObjFactory
 *    subEVT: ModuleStartEvent, ModuleStopEvent
 *    postEVT: ModuleAliveEvent, ModuleCreateEvent, ModuleDestroyEvent
 */

/**
 * @details CliObj
 *    subEVT: ModuleStartEvent, ModuleStopEvent, SrvOpen[HiRes/LoRes]StreamEvent, SrvCloseStreamEvent
 *    postEVT: ModuleAliveEvent, SrvIdentifyClient, SrvVidOnlyEvent, SrvAudOnlyEvent, SrvStreamAliveEvent
 */

#include "_UT_IOC_Common.h"