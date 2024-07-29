/**
 * @file UT_ConlesEventDemoLiveCam.cxx
 * @brief Demo for ConlesEvent of LiveStreamingCamera(a.k.a LiveCam).
 * @details
 *  @[API] LiveCam use ConlesEvent to simulate the behaviors of LiveCam's modules.
 *   a) IOC_subEVT_inConlesMode(useCbProcEvt_F), IOC_unsubEVT_inConlesMode
 *   b) IOC_postEVT_inConlesMode
 *  @[Func] LiveCam has a network service module which used to accept the connection from the client.
 *   a) Client will receive the camera's live audio&video stream by default,
 *       client may select audio-only, video-only, or both after connected.
 *   b) Only VIP client may continuly receive high-resolution(HiRes) audio&video stream,
 *       others will receive HiRes in first 5 minutes then switch to low-resolution(LoRes) audio&video stream.
 *   c) Only a VIP client may request bidirectional audio&video communication.
 *  @[Module] LiveCam also internally include modules for video/audio capture, video/audio encode, stream multiplexing(mux).
 *   a) video capture in HiRes by default, and may use video resize module to get LoRes video.
 */

/**
 * @version <DOING>0.1.0: initial version, only LoRes srv->cli stream.
 * @version <TODO>0.1.1: VIP client may receive HiRes srv->cli stream.
 * @version <TODO>0.1.2: VIP client may request bidirectional stream.
 */

//======>>>>>>BEGIN OF PRIMITIVE UT DESIGN<<<<<<<=======================================================================
/**
 * ----> ServerSide:
 * ModMgrObj: all modules in LiveCam is managed by ModMgrObj,
 *      which means created/destroyed/started/stopped by ModMgrObj,
 *      which also means all modules MUST post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 * SrvObj: ServerObject, used to simulate the server behaviors.
 * HiResStrmMuxObj: High Resolution Stream Object, used to simulate the HiRes stream.
 * LoResStrmMuxObj: Low Resolution Stream Object, used to simulate the LoRes stream.
 * HiResVidEncObj: High Resolution Video Encode Object, used to simulate the HiRes video encode.
 * LoResVidEncObj: Low Resolution Video Encode Object, used to simulate the LoRes video encode.
 * VidResizeObj: Video Resize Object, used to simulate the video resize.
 * AudEncObj: Audio Encode Object, used to simulate the audio encode.
 * VidCapObj: Video Capture Object, used to simulate the video capture.
 * AudCapObj: Audio Capture Object, used to simulate the audio capture.
 *---------------------------------------------------------------------------------------------------------------------
 *---> ClientSide:
 * CliObjFactory: ClientObject Factory, used to create ClientObject.
 *  |-> CliObj: ClientObject, created by CliObjFactory, used to simulate the client behaviors.
 */

/**
 * RefMore: UT_ConlesEventDemoLiveCam.md
 *   Data flow and event flow between LiveCam's module objects.
 */

/**
 * @details ModMgrObj(Created by MAIN)
 * @behaviros:
 *    1) ModMgrObj will create all server side modules in LiveCam.
 *        a) start/stop all modules in LiveCam.
 *        b) destroy all modules in LiveCam.
 *    2) ModMgrObj will diagnose all server side modules in LiveCam.
 *        a) check all module is alive.
 *        b) check all module's BizSpec meeted.
 *              such as VidCapObj's BizSpec is "capture high resolution video frame in 30fps".
 * @todo
 *    subEVT: ModuleKeepAliveEvent, BizXyzEvent(each BizSpec has diagnosiable metrics)
 *    postEVT: Module[Start,Stop]Event
 */

/**
 * @brief VidCapObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuCapture video frame in 1920x1080@30fps(a.k.a OriVidFrm) and send to HiResVidEncObj or VidResizeObj.
 *      a) HiResVidEncObj or VidResizeObj will reference video frame,
 *          so wait video frame recycle event from HiResVidEncObj or VidResizeObj.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizOriVidFrmRecycledEvent
 *    postEVT: ModuleKeepAliveEvent, BizOriVidFrmCapturedEvent
 */

/**
 * @brief HiResVidEncObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuEncode video frame to video stream bits(a.k.a HiResVidStrmBits) in 1920x1080@30fps and send to HiResStrmMuxObj.
 *      a) HiResStrmMuxObj will reference HiResVidStrmBits,
 *          so wait HiResVidStrmBits recycle event from HiResStrmMuxObj.
 *      b) recycle HiResVidFrm to VidCapObj after HiResVidStrmBits encoded or sended to HiResStrmMuxObj.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizOriVidFrmCapturedEvent, BizHiResVidStrmBitsRecycledEvent
 *    postEVT: ModuleKeepAliveEvent, BizOriVidFrmRecycledEvent, BizHiResVidStrmBitsEncodedEvent
 */

/**
 * @brief HiResStrmMuxObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuMux HiResVidStrmBits and AudStrmBits to a HiResStrmBits in 1920x1080@30fps and 48KHz@16bit.
 *      a) send HiResStrmBits to SrvObj which will send to client.
 *      b) recycle HiResVidStrmBits to HiResVidEncObj.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizHiResVidStrmBitsEncodedEvent, BizAudStrmBitsEncodedEvent,
 *      BizHiResStrmBitsRecycledEvent
 *    postEVT: ModuleKeepAliveEvent, BizHiResStrmBitsMuxedEvent, BizHiResVidStrmBitsRecycledEvent
 */

/**
 * @brief SrvObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuAccept client connection and send LoRes stream to client.
 *      a) v0.1.1: send HiResStrmBits to client by default, switch to LoResStrmBits if not VIP after 5 minutes.
 *      b) v0.1.2: accept bidirectional stream request from VIP client.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizHiResStrmBitsMuxedEvent, BizLoResStrmBitsMuxedEvent,
 *      SrvOpenStreamEvent, SrvCloseStreamEvent
 *    postEVT: ModuleKeepAliveEvent, BizHiResStrmBitsRecycledEvent, BizLoResStrmBitsRecycledEvent,
 *      BizHiResStrmBitsSentEvent, BizLoResStrmBitsSentEvent
 */

/**
 * @brief VidResizeObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuResize OriVidFrm into LoResVidFrm of 640x480@30fps and send to LoResVidEncObj.
 *      a) LoResVidEncObj will reference LoResVidFrm,
 *          so wait video frame recycle event from LoResVidEncObj.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizOriVidFrmCapturedEvent, BizLoResVidFrmRecycledEvent
 *    postEVT: ModuleKeepAliveEvent, BizOriVidFrmRecycledEvent, BizLoResVidFrmResizedEvent
 */

/**
 * @brief LoResVidEncObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuEncode LoResVidFrm to LoResVidStrmBits in 640x480@30fps and send to LoResStrmMuxObj.
 *      a) LoResStrmMuxObj will reference video stream bits,
 *          so wait video stream bits recycle event from LoResStrmMuxObj.
 *      b) recycle video frame to VidResizeObj after video stream bits encoded or send to LoResStrmMuxObj.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizLoResVidFrmResizedEvent, BizLoResVidStrmBitsRecycledEvent
 *    postEVT: ModuleKeepAliveEvent, BizLoResVidStrmBitsEncodedEvent, BizLoResVidFrmRecycledEvent
 *
 */

/**
 * @brief LoResStrmMuxObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuMux LoResVidStrmBits and AudStrmBits to a LoResStrmBits in 640x480@30fps and 48KHz@16bit.
 *      a) send LoResStrmBits to SrvObj which will send to client.
 *      b) recycle LoResVidStrmBits to LoResVidEncObj.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizLoResVidStrmBitsEncodedEvent, BizAudStrmBitsEncodedEvent
 *      BizLoResStrmBitsRecycledEvent
 *    postEVT: ModuleKeepAliveEvent, BizLoResStrmBitsMuxedEvent, BizLoResVidStrmBitsRecycledEvent
 */

/**
 * @brief AudCapObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuCapture audio frame in 48KHz@16bit and post to AudEncObj.
 *      a) AudEncObj will copy audio frame data,
 *          so don't wait audio frame recycle event from AudEncObj.
 *    3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent
 *    postEVT: ModuleKeepAliveEvent, BizOriAudFrmCapturedEvent
 */

/**
 * @brief AudEncObj
 * @behaviors:
 *    1) created by ModMgrObj and wait for start event.
 *    2) simuEncode audio frame to audio stream bits(a.k.a AudStrmBits) in 48KHz@16bit and send to HiResStrmMuxObj or
 * LoResStrmMuxObj. a) HiResStrmMuxObj or LoResStrmMuxObj will copy audio stream bits, so don't wait audio stream bits recycle
 * event from HiResStrmMuxObj or LoResStrmMuxObj. 3) post ModuleKeepAliveEvent to ModMgrObj in 1s interval.
 *
 * @todo
 *    subEVT: ModuleStartEvent, ModuleStopEvent, BizOriAudFrmCapturedEvent
 *    postEVT: ModuleKeepAliveEvent, BizAudStrmBitsEncodedEvent
 *
 */

/**
 * @details CliObjFactory(Created by MAIN)
 * @behaviors:
 *    1) create CliObj by predefined strategy.
 *    2) destroy CliObj by predefined strategy.
 *
 * @todo
 *    subEVT: CliKeepAliveEvent
 *    postEVT: CliStartEvent, CliStopEvent
 */

/**
 * @details CliObj
 *    subEVT: CliStartEvent, CliStopEvent, BizHiResStrmBitsSentEvent, BizLoResStrmBitsSentEvent
 *    postEVT: CliKeepAliveEvent, SrvOpenStreamEvent, SrvCloseStreamEvent
 */
//======>>>>>>END OF PRIMITIVE UT DESIGN<<<<<<<========================================================================

//======>>>>>>BEGIN OF UT DEFINATION<<<<<<<=============================================================================
#include <ctime>

#include "_UT_IOC_Common.h"
// Define DemoLiveCam's Event Class use IOC_EVT_CLASS_TEST
#define IOC_EVT_CLASS_LIVECAM IOC_EVT_CLASS_TEST

// Define DemoLiveCam's Event Name by events between LiveCam's module objects.
typedef enum {
  // RefEvent flow of service side module objects from Biz Viewpoint
  IOC_ENAME_BizOriVidFrmCaptured,  // 0
  IOC_ENAME_BizOriVidFrmRecycled,
  IOC_ENAME_BizHiResVidStrmBitsEncoded,
  IOC_ENAME_BizHiResVidStrmBitsRecycled,
  IOC_ENAME_BizHiResStrmBitsMuxed,
  IOC_ENAME_BizHiResStrmBitsRecycled,  // 5

  IOC_ENAME_BizLoResVidFrmResized,
  IOC_ENAME_BizLoResVidFrmRecycled,
  IOC_ENAME_BizLoResVidStrmBitsEncoded,
  IOC_ENAME_BizLoResVidStrmBitsRecycled,
  IOC_ENAME_BizLoResStrmBitsMuxed,  // 10
  IOC_ENAME_BizLoResStrmBitsRecycled,

  IOC_ENAME_BizOriAudFrmCaptured,
  IOC_ENAME_BizAudStrmBitsEncoded,

  // RefEvent flow of service side module objects from Biz Viewpoint
  IOC_ENAME_ModStart,
  IOC_ENAME_ModStop,
  IOC_ENAME_ModKeepAlive,

  // RefEvent flow between server and client side module objects
  IOC_ENAME_SrvOpenStream,
  IOC_ENAME_SrvCloseStream,
  IOC_ENAME_BizHiResStrmBitsSent,
  IOC_ENAME_BizLoResStrmBitsSent,

  // RefEvent flow of client side module objects
  IOC_ENAME_CliStart,
  IOC_ENAME_CliStop,
  IOC_ENAME_CliKeepAlive,

} IOC_EvtNameDemoLiveCam_T;

// Define DemoLiveCam's Event ID by event flow between LiveCam's module objects from Biz Viewpoint
#define IOC_EVTID_BizOriVidFrmCaptured IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizOriVidFrmCaptured)
#define IOC_EVTID_BizOriVidFrmRecycled IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizOriVidFrmRecycled)
#define IOC_EVTID_BizHiResVidStrmBitsEncoded IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizHiResVidStrmBitsEncoded)
#define IOC_EVTID_BizHiResVidStrmBitsRecycled IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizHiResVidStrmBitsRecycled)
#define IOC_EVTID_BizHiResStrmBitsMuxed IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizHiResStrmBitsMuxed)
#define IOC_EVTID_BizHiResStrmBitsRecycled IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizHiResStrmBitsRecycled)

#define IOC_EVTID_BizLoResVidFrmResized IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizLoResVidFrmResized)
#define IOC_EVTID_BizLoResVidFrmRecycled IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizLoResVidFrmRecycled)
#define IOC_EVTID_BizLoResVidStrmBitsEncoded IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizLoResVidStrmBitsEncoded)
#define IOC_EVTID_BizLoResVidStrmBitsRecycled IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizLoResVidStrmBitsRecycled)
#define IOC_EVTID_BizLoResStrmBitsMuxed IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizLoResStrmBitsMuxed)
#define IOC_EVTID_BizLoResStrmBitsRecycled IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizLoResStrmBitsRecycled)

#define IOC_EVTID_BizOriAudFrmCaptured IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizOriAudFrmCaptured)
#define IOC_EVTID_BizAudStrmBitsEncoded IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizAudStrmBitsEncoded)

// Define DemoLiveCam's Event ID by event flow between LiveCam's module objects from Management Viewpoint
#define IOC_EVTID_ModStart IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_ModStart)
#define IOC_EVTID_ModStop IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_ModStop)
#define IOC_EVTID_ModKeepAlive IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_ModKeepAlive)

#define IOC_EVTID_SrvOpenStream IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_SrvOpenStream)
#define IOC_EVTID_SrvCloseStream IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_SrvCloseStream)
#define IOC_EVTID_BizHiResStrmBitsSent IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizHiResStrmBitsSent)
#define IOC_EVTID_BizLoResStrmBitsSent IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_BizLoResStrmBitsSent)

#define IOC_EVTID_CliStart IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_CliStart)
#define IOC_EVTID_CliStop IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_CliStop)
#define IOC_EVTID_CliKeepAlive IOC_defineEvtID(IOC_EVT_CLASS_LIVECAM, IOC_ENAME_CliKeepAlive)

typedef enum {
    ObjState_Stopped,  // stopped state
                       // initial state or stopped by user on IOC_EVTID_ModStop from running state
    ObjState_Running,  // running state
                       // started by user on IOC_EVTID_ModStart from stopped state
} _LiveCamObjState_T;

typedef enum {
    ObjID_VidCapObj,
    ObjID_AudCapObj,
    ObjID_HiResVidEncObj,
    ObjID_LoResVidEncObj,
    ObjID_VidResizeObj,
    ObjID_HiResStrmMuxObj,
    ObjID_LoResStrmMuxObj,
    ObjID_AudEncObj,
    ObjID_SrvObj,
    ObjID_ModMgrObj,

    ObjID_CliObjFactory,
    ObjID_CliObj_0,
    ObjID_CliObj_1,
    ObjID_CliObj_2,
    ObjID_CliObj_3,
    ObjID_CliObj_4,
    ObjID_CliObj_5,
    ObjID_CliObj_6,
    ObjID_CliObj_7,
    ObjID_CliObj_8,
    ObjID_CliObj_9,

} _LiveCamObjID_T;

// RefBrief: server and client side module objects
typedef struct {
    _LiveCamObjID_T ObjID;
    _LiveCamObjState_T State;

    /**
     * @initial: NOW=IOC_getCurrentTimeSpec()
     * @Every1s: UPDATE=IOC_getCurrentTimeSpec()
     */
    struct timespec LastKeepAliveTime;
    ULONG_T KeepAliveCnt;
} _LiveCamObjBase_T, *_LiveCamObjBase_pT;

static void __postKeepAliveEvt(_LiveCamObjBase_T *pBaseObj) {
    // post ModuleKeepAliveEvent to ModMgrObj in 1s interval.

    struct timespec Now = IOC_getCurrentTimeSpec();
    if (IOC_diffTimeSpecInSec(&pBaseObj->LastKeepAliveTime, &Now) >= 1) {
      IOC_EvtDesc_T EvtDesc = {
          .EvtID    = IOC_EVTID_ModKeepAlive,
          .EvtValue = static_cast<ULONG_T>(pBaseObj->ObjID),
      };

      IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
      EXPECT_EQ(Result, IOC_RESULT_SUCCESS);

      pBaseObj->KeepAliveCnt++;
      pBaseObj->LastKeepAliveTime = Now;
    }
}

// RefBrief: VidCapObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;           // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;            // TotalMgntSpecEvents
        ULONG_T BizOriVidFrmRecycledEvent;  // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizOriVidFrmCapturedEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

    pthread_t SimuVidCapThread;

} _LiveCamVidCapObj_T, *_LiveCamVidCapObj_pT;

// RefBrief: AudCapObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;  // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;   // TotalMgntSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizOriAudFrmCapturedEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

    pthread_t SimuAudCapThread;

} _LiveCamAudCapObj_T, *_LiveCamAudCapObj_pT;

// RefBrief: AudEncObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;           // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;            // TotalMgntSpecEvents
        ULONG_T BizOriAudFrmCapturedEvent;  // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizAudStrmBitsEncodedEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamAudEncObj_T, *_LiveCamAudEncObj_pT;

// RefBrief: HiResVidEncObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;                  // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;                   // TotalMgntSpecEvents
        ULONG_T BizOriVidFrmCapturedEvent;         // TotalBizSpecEvents
        ULONG_T BizHiResVidStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizHiResVidStrmBitsEncodedEvent;  // TotalBizSpecEvents
        ULONG_T BizOriVidFrmRecycledEvent;        // TotalBizSpecEvents
    } TotalPostEvents;
} _LiveCamHiResVidEncObj_T, *_LiveCamHiResVidEncObj_pT;

// RefBrief: LoResVidEncObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;                  // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;                   // TotalMgntSpecEvents
        ULONG_T BizLoResVidFrmResizedEvent;        // TotalBizSpecEvents
        ULONG_T BizLoResVidStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizLoResVidStrmBitsEncodedEvent;  // TotalBizSpecEvents
        ULONG_T BizLoResVidFrmRecycledEvent;      // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamLoResVidEncObj_T, *_LiveCamLoResVidEncObj_pT;

// RefBrief: VidResizeObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;           // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;            // TotalMgntSpecEvents
        ULONG_T BizOriVidFrmCapturedEvent;  // TotalBizSpecEvents
        ULONG_T BizLoResVidFrmRecycledEvent;  // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizOriVidFrmRecycledEvent;   // TotalBizSpecEvents
        ULONG_T BizLoResVidFrmResizedEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamVidResizeObj_T, *_LiveCamVidResizeObj_pT;

// RefBrief: HiResStrmMuxObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;                 // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;                  // TotalMgntSpecEvents
        ULONG_T BizHiResVidStrmBitsEncodedEvent;  // TotalBizSpecEvents
        ULONG_T BizAudStrmBitsEncodedEvent;       // TotalBizSpecEvents
        ULONG_T BizHiResStrmBitsRecycledEvent;    // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizHiResStrmBitsMuxedEvent;        // TotalBizSpecEvents
        ULONG_T BizHiResVidStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamHiResStrmMuxObj_T, *_LiveCamHiResStrmMuxObj_pT;

// RefBrief: LoResStrmMuxObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;                 // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;                  // TotalMgntSpecEvents
        ULONG_T BizLoResVidStrmBitsEncodedEvent;  // TotalBizSpecEvents
        ULONG_T BizAudStrmBitsEncodedEvent;       // TotalBizSpecEvents
        ULONG_T BizLoResStrmBitsRecycledEvent;    // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizLoResStrmBitsMuxedEvent;        // TotalBizSpecEvents
        ULONG_T BizLoResVidStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamLoResStrmMuxObj_T, *_LiveCamLoResStrmMuxObj_pT;

// RefBrief: SrvObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;            // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;             // TotalMgntSpecEvents
        ULONG_T BizHiResStrmBitsMuxedEvent;  // TotalBizSpecEvents
        ULONG_T BizLoResStrmBitsMuxedEvent;  // TotalBizSpecEvents
        ULONG_T SrvOpenStreamEvent;          // TotalBizSpecEvents
        ULONG_T SrvCloseStreamEvent;         // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizHiResStrmBitsSentEvent;      // TotalBizSpecEvents
        ULONG_T BizLoResStrmBitsSentEvent;      // TotalBizSpecEvents
        ULONG_T BizHiResStrmBitsRecycledEvent;  // TotalBizSpecEvents
        ULONG_T BizLoResStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamSrvObj_T, *_LiveCamSrvObj_pT;

// RefBrief: ModMgrObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT: ModuleKeepAliveEvent
    struct {
        ULONG_T VidCapObj;
        ULONG_T AudCapObj;
        ULONG_T HiResVidEncObj;
        ULONG_T LoResVidEncObj;
        ULONG_T VidResizeObj;
        ULONG_T HiResStrmMuxObj;
        ULONG_T LoResStrmMuxObj;
        ULONG_T AudEncObj;
        ULONG_T SrvObj;
    } TotalKeepAliveEvents;  // TotalMgntSpecEvents

} _LiveCamModMgrObj_T, *_LiveCamModMgrObj_pT;

// RefBrief: CliObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T CliStartEvent;              // TotalMgntSpecEvents
        ULONG_T CliStopEvent;               // TotalMgntSpecEvents
        ULONG_T BizHiResStrmBitsSentEvent;  // TotalBizSpecEvents
        ULONG_T BizLoResStrmBitsSentEvent;  // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T CliKeepAliveEvent;    // TotalMgntSpecEvents
        ULONG_T SrvOpenStreamEvent;   // TotalBizSpecEvents
        ULONG_T SrvCloseStreamEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamCliObj_T, *LiveCamCliObj_pT;

// RefBrief: CliObjFactory
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T CliKeepAliveEvent;  // TotalMgntSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T CliStartEvent;  // TotalMgntSpecEvents
        ULONG_T CliStopEvent;   // TotalMgntSpecEvents
    } TotalPostEvents;

} _LiveCamCliObjFactory_T, *LiveCamCliObjFactory_pT;

//======>>>>>>END OF UT DEFINATION<<<<<<<===============================================================================

//======>>>>>>BEGIN OF UT IMPLEMENTATION<<<<<<<=========================================================================

/**
 * @[Name]: No1::UT_ConlesEventDemoLiveCam_verifyFunctionality_v0_1_0
 * @[Purpose]: accroding to v0.1.0, verify the functionality of ConlesEventDemoLiveCam,
 *  which means only LoRes stream will be sent to client.
 * @[Overview]:
 *    a) ViCapObj: 1920x1080@25fps x 100s -> BizOriVidFrmCapturedEvent/40ms
 *          |-> TotalBizOriVidFrmCapturedEvents = 25x100 = 2500
 *    b) AudCapObj: 8KHz@16bit x 100s -> BizOriAudFrmCapturedEvent/20ms
 *          |-> TotalBizOriAudFrmCapturedEvents = 50x100 = 5000
 *    c) CliObj: 5xcurrent clients, each receive (V25fps+A50fps)x9s
 * @[Steps]:
 *  1) create all server side module objects, and subscribe each module's events when created,
 *        then start all server side module objects as SETUP&BEHAVIOR.
 *  2) create client object factory,
 *        which will create 5 client objects every 10s of total 9 times as SETUP&BEHAVIOR.
 *  3) wait for 101s, then stop all server side module objects.
 *        check each object's EVTCNT of BizSpec events as VERIFY.
 *  4) destroy all server and client side objects as CLEANUP.
 * @[Expects]:
 *  All EVTCNT of BizSpec events meet the expected value, such as
 *      ViCapObj's EVTCNT of BizOriVidFrmCapturedEvent is 2500,
 *      AudCapObj's EVTCNT of BizOriAudFrmCapturedEvent is 5000.
 *  All EVTCNT of MgntSpec events meet the expected value, such as
 *      ViCapObj's EVTCNT of KeepAliveEvent is 100,
 *      AudCapObj's EVTCNT of KeepAliveEvent is 100.
 * @[Notes]: N/A
 */
#define _CASE01_DURATION 10  // NOLINT(*-reserved-identifier)

#define _CASE01_VIDCAP_FPS 25
#define _CASE01_VIDCAP_FRM_CNT (_CASE01_DURATION * _CASE01_VIDCAP_FPS)

#define _CASE01_AUDCAP_FPS 50
#define _CASE01_AUDCAP_FRM_CNT (_CASE01_DURATION * _CASE01_AUDCAP_FPS)

#define _CASE01_PERF_LE_3000US 3000UL
static void __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(IOC_EvtDesc_pT pEvtDesc) {
    struct timespec CurrentTime = IOC_getCurrentTimeSpec();

    ULONG_T TimeCost = IOC_diffTimeSpecInUS(&pEvtDesc->MsgDesc.TimeStamp, &CurrentTime);
    EXPECT_LE(TimeCost, _CASE01_PERF_LE_3000US)
        << "TimeCost=" << TimeCost << "us, expect less than " << _CASE01_PERF_LE_3000US << "us";
}

static void *__Case01_ThreadFunc_simuVidCap(void *arg) {
    _LiveCamVidCapObj_T *pVidCapObj = (_LiveCamVidCapObj_T *)arg;
    struct timespec LastCaptureTime = IOC_getCurrentTimeSpec();
    struct timespec CurrentTime;
    struct timespec SleepTime;

    while (pVidCapObj->Base.State == ObjState_Running) {
        clock_gettime(CLOCK_MONOTONIC, &CurrentTime);
        if (IOC_diffTimeSpecInMS(&LastCaptureTime, &CurrentTime) >= 40) {
            // simulate capture video frame in 1920x1080@25fps
            // 1: post the BizOriVidFrmCapturedEvent
            IOC_EvtDesc_T EvtDesc = {
                .EvtID = IOC_EVTID_BizOriVidFrmCaptured,
            };

            IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
            EXPECT_EQ(IOC_RESULT_SUCCESS, Result)
                << "postEvt(BizOriVidFrmCapturedEvent) fail, Result=" << IOC_getResultStr(Result);

            // 2: updateStatistics
            pVidCapObj->TotalPostEvents.BizOriVidFrmCapturedEvent++;  // postEvt: BizOriVidFrmCapturedEvent
            LastCaptureTime = CurrentTime;

            // 3: tell ModMgr Iam alive
            __postKeepAliveEvt(&pVidCapObj->Base);
        }

        if (pVidCapObj->TotalPostEvents.BizOriVidFrmCapturedEvent >= _CASE01_VIDCAP_FRM_CNT) {
            // stop the simu looping, and exit the thread
            break;
        } else {
            // sleep 1ms
            SleepTime.tv_sec  = 0;
            SleepTime.tv_nsec = 1000000;
            nanosleep(&SleepTime, NULL);
        }
    }

    return NULL;
}

static IOC_Result_T __Case01_cbProcEvt_VidCapObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamVidCapObj_pT pVidCapObj = (_LiveCamVidCapObj_T *)pCbPriv;
    IOC_Result_T Result             = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID               = pEvtDesc->EvtID;

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    switch (EvtID) {
        case IOC_EVTID_ModStart: {
            if (pVidCapObj->Base.State == ObjState_Stopped) {
                pVidCapObj->Base.State = ObjState_Running;

                // create simu capture thread
                int RetPSX = pthread_create(&pVidCapObj->SimuVidCapThread, NULL, __Case01_ThreadFunc_simuVidCap, pVidCapObj);
                EXPECT_EQ(0, RetPSX);

                pVidCapObj->TotalSubEvents.ModuleStartEvent++;
                Result = IOC_RESULT_SUCCESS;

            } else {
                // BUG: expect stopped state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect stopped state, but " << pVidCapObj->Base.State;
            }
        } break;

        case IOC_EVTID_ModStop: {
            if (pVidCapObj->Base.State == ObjState_Running) {
                pVidCapObj->Base.State = ObjState_Stopped;

                // join simu capture thread
                int RetPSX = pthread_join(pVidCapObj->SimuVidCapThread, NULL);
                EXPECT_EQ(0, RetPSX);

                pVidCapObj->TotalSubEvents.ModuleStopEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pVidCapObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizOriVidFrmRecycled: {
            if (pVidCapObj->Base.State == ObjState_Running) {
                pVidCapObj->TotalSubEvents.BizOriVidFrmRecycledEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pVidCapObj->Base.State;
            }
        } break;

        default: {
            // BUG: unexpected event
            EXPECT_FALSE(true) << "BUG: unexpected event " << EvtID;
        } break;
    }

    return Result;
}

static void __Case01_setupVidCapObj(_LiveCamVidCapObj_pT pVidCapObj) {
    // SETUP ZERO AS DEFAULT
    memset(pVidCapObj, 0, sizeof(_LiveCamVidCapObj_T));

    pVidCapObj->Base.ObjID             = ObjID_VidCapObj;
    pVidCapObj->Base.State             = ObjState_Stopped;
    pVidCapObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_ModStart,
        IOC_EVTID_ModStop,
        IOC_EVTID_BizOriVidFrmRecycled,
    };
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_VidCapObj,
        .pCbPrivData = pVidCapObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupVidCapObj(_LiveCamVidCapObj_pT pVidCapObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_VidCapObj,
        .pCbPrivData = pVidCapObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "VidCapObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_verifyVidCapObj(_LiveCamVidCapObj_pT pVidCapObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, pVidCapObj->Base.State);
    EXPECT_EQ(_CASE01_DURATION, pVidCapObj->Base.KeepAliveCnt);

    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pVidCapObj->TotalPostEvents.BizOriVidFrmCapturedEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pVidCapObj->TotalSubEvents.BizOriVidFrmRecycledEvent);

    EXPECT_EQ(1, pVidCapObj->TotalSubEvents.ModuleStartEvent);
    EXPECT_EQ(1, pVidCapObj->TotalSubEvents.ModuleStopEvent);
}

static IOC_Result_T __Case01_cbProcEvt_ModMgrObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamModMgrObj_pT pModMgrObj = (_LiveCamModMgrObj_pT)pCbPriv;
    IOC_Result_T Result             = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID               = pEvtDesc->EvtID;

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    // ModMgrObj's state MUST be running
    if (pModMgrObj->Base.State != ObjState_Running) {
        // BUG: expect running state, but ??? state
        EXPECT_FALSE(true) << "BUG: expect running state, but " << pModMgrObj->Base.State;
        return Result;
    }

    // process event KeepAlive only
    if (EvtID == IOC_EVTID_ModKeepAlive) {
        _LiveCamObjID_T ObjID = (_LiveCamObjID_T)pEvtDesc->EvtValue;
        switch (ObjID) {
            case ObjID_VidCapObj: {
                pModMgrObj->TotalKeepAliveEvents.VidCapObj++;
            } break;

            case ObjID_AudCapObj: {
                pModMgrObj->TotalKeepAliveEvents.AudCapObj++;
            } break;

            case ObjID_HiResVidEncObj: {
                pModMgrObj->TotalKeepAliveEvents.HiResVidEncObj++;
            } break;

            case ObjID_LoResVidEncObj: {
                pModMgrObj->TotalKeepAliveEvents.LoResVidEncObj++;
            } break;

            case ObjID_VidResizeObj: {
                pModMgrObj->TotalKeepAliveEvents.VidResizeObj++;
            } break;

            case ObjID_HiResStrmMuxObj: {
                pModMgrObj->TotalKeepAliveEvents.HiResStrmMuxObj++;
            } break;

            case ObjID_LoResStrmMuxObj: {
                pModMgrObj->TotalKeepAliveEvents.LoResStrmMuxObj++;
            } break;

            case ObjID_AudEncObj: {
                pModMgrObj->TotalKeepAliveEvents.AudEncObj++;
            } break;

            case ObjID_SrvObj: {
                pModMgrObj->TotalKeepAliveEvents.SrvObj++;
            } break;

            default: {
                // BUG: unexpected object
                EXPECT_FALSE(true) << "BUG: unexpected object " << ObjID;
            } break;
        }

        Result = IOC_RESULT_SUCCESS;
    }

    return Result;
}

static void __Case01_setupModMgrObj(_LiveCamModMgrObj_T *ModMgrObj) {
    // SETUP ZERO AS DEFAULT
    memset(ModMgrObj, 0, sizeof(_LiveCamModMgrObj_T));

    ModMgrObj->Base.ObjID             = ObjID_ModMgrObj;
    ModMgrObj->Base.State             = ObjState_Stopped;
    ModMgrObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[]     = {IOC_EVTID_ModKeepAlive};
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_ModMgrObj,
        .pCbPrivData = ModMgrObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupModMgrObj(_LiveCamModMgrObj_T *ModMgrObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_ModMgrObj,
        .pCbPrivData = ModMgrObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "ModMgrObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_startModMgrObj(_LiveCamModMgrObj_T *ModMgrObj) {
    // update self state to running firstly
    ModMgrObj->Base.State = ObjState_Running;

    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_ModStart,
    };

    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_stopModMgrObj(_LiveCamModMgrObj_T *ModMgrObj) {
    IOC_EvtDesc_T EvtDesc = {
        .EvtID = IOC_EVTID_ModStop,
    };

    IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

    // update self state to stopped lastly
    ModMgrObj->Base.State = ObjState_Stopped;
}

static void __Case01_verifyModMgrObj(_LiveCamModMgrObj_T *ModMgrObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, ModMgrObj->Base.State);

    EXPECT_EQ(_CASE01_DURATION, ModMgrObj->TotalKeepAliveEvents.VidCapObj);
    EXPECT_EQ(_CASE01_DURATION, ModMgrObj->TotalKeepAliveEvents.AudCapObj);
    EXPECT_EQ(_CASE01_DURATION, ModMgrObj->TotalKeepAliveEvents.LoResVidEncObj);
    EXPECT_EQ(_CASE01_DURATION, ModMgrObj->TotalKeepAliveEvents.VidResizeObj);
    EXPECT_EQ(_CASE01_DURATION, ModMgrObj->TotalKeepAliveEvents.LoResStrmMuxObj);
    EXPECT_EQ(_CASE01_DURATION, ModMgrObj->TotalKeepAliveEvents.AudEncObj);
    EXPECT_EQ(_CASE01_DURATION, ModMgrObj->TotalKeepAliveEvents.SrvObj);
}

static void *__Case01_ThreadFunc_simuAudCap(void *arg) {
    _LiveCamAudCapObj_T *pAudCapObj = (_LiveCamAudCapObj_T *)arg;
    struct timespec LastCaptureTime = IOC_getCurrentTimeSpec();
    struct timespec CurrentTime;
    struct timespec SleepTime;

    while (pAudCapObj->Base.State == ObjState_Running) {
        clock_gettime(CLOCK_MONOTONIC, &CurrentTime);
        if (IOC_diffTimeSpecInMS(&LastCaptureTime, &CurrentTime) >= 20) {
            // simulate capture audio frame in 8KHz@16bit
            // 1: post the BizOriAudFrmCapturedEvent
            IOC_EvtDesc_T EvtDesc = {
                .EvtID = IOC_EVTID_BizOriAudFrmCaptured,
            };

            IOC_Result_T Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
            EXPECT_EQ(IOC_RESULT_SUCCESS, Result)
                << "postEvt(BizOriAudFrmCapturedEvent) fail, Result=" << IOC_getResultStr(Result);

            // 2: updateStatistics
            pAudCapObj->TotalPostEvents.BizOriAudFrmCapturedEvent++;  // postEvt: BizOriAudFrmCapturedEvent
            LastCaptureTime = CurrentTime;

            // 3: tell ModMgr Iam alive
            __postKeepAliveEvt(&pAudCapObj->Base);
        }

        if (pAudCapObj->TotalPostEvents.BizOriAudFrmCapturedEvent >= _CASE01_AUDCAP_FRM_CNT) {
            // stop the simu looping, and exit the thread
            break;
        } else {
            // sleep 1ms
            SleepTime.tv_sec  = 0;
            SleepTime.tv_nsec = 1000000;
            nanosleep(&SleepTime, NULL);
        }
    }

    return NULL;
}

static IOC_Result_T __Case01_cbProcEvt_AudCapObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamAudCapObj_T *pAudCapObj = (_LiveCamAudCapObj_T *)pCbPriv;
    IOC_Result_T Result             = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID               = pEvtDesc->EvtID;

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    switch (EvtID) {
        case IOC_EVTID_ModStart: {
            if (pAudCapObj->Base.State == ObjState_Stopped) {
                pAudCapObj->Base.State = ObjState_Running;

                // create simu audio capture thread
                int RetPSX = pthread_create(&pAudCapObj->SimuAudCapThread, NULL, __Case01_ThreadFunc_simuAudCap, pAudCapObj);
                EXPECT_EQ(0, RetPSX);

                pAudCapObj->TotalSubEvents.ModuleStartEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect stopped state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect stopped state, but " << pAudCapObj->Base.State;
            }
        } break;

        case IOC_EVTID_ModStop: {
            if (pAudCapObj->Base.State == ObjState_Running) {
                pAudCapObj->Base.State = ObjState_Stopped;

                // join simu audio capture thread
                int RetPSX = pthread_join(pAudCapObj->SimuAudCapThread, NULL);
                EXPECT_EQ(0, RetPSX);

                pAudCapObj->TotalSubEvents.ModuleStopEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pAudCapObj->Base.State;
            }
        } break;

        default: {
            // BUG: unexpected event
            EXPECT_FALSE(true) << "BUG: unexpected event " << EvtID;
        } break;
    }

    return Result;
}

static void __Case01_setupAudCapObj(_LiveCamAudCapObj_pT AudCapObj) {
    // SETUP ZERO AS DEFAULT
    memset(AudCapObj, 0, sizeof(_LiveCamAudCapObj_T));

    AudCapObj->Base.ObjID             = ObjID_AudCapObj;
    AudCapObj->Base.State             = ObjState_Stopped;
    AudCapObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_ModStart,
        IOC_EVTID_ModStop,
    };
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_AudCapObj,
        .pCbPrivData = AudCapObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupAudCapObj(_LiveCamAudCapObj_pT pAudCapObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_AudCapObj,
        .pCbPrivData = pAudCapObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "AudCapObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_verifyAudCapObj(_LiveCamAudCapObj_pT pAudCapObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, pAudCapObj->Base.State);
    EXPECT_EQ(_CASE01_DURATION, pAudCapObj->Base.KeepAliveCnt);
    EXPECT_EQ(_CASE01_AUDCAP_FRM_CNT, pAudCapObj->TotalPostEvents.BizOriAudFrmCapturedEvent);

    EXPECT_EQ(1, pAudCapObj->TotalSubEvents.ModuleStartEvent);
    EXPECT_EQ(1, pAudCapObj->TotalSubEvents.ModuleStopEvent);
}

static IOC_Result_T __Case01_cbProcEvt_AudEncObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamAudEncObj_pT pAudEncObj = (_LiveCamAudEncObj_pT)pCbPriv;
    IOC_Result_T Result             = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID               = pEvtDesc->EvtID;

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    switch (EvtID) {
        case IOC_EVTID_ModStart: {
            if (pAudEncObj->Base.State == ObjState_Stopped) {
                pAudEncObj->Base.State = ObjState_Running;

                pAudEncObj->TotalSubEvents.ModuleStartEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect stopped state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect stopped state, but " << pAudEncObj->Base.State;
            }
        } break;

        case IOC_EVTID_ModStop: {
            if (pAudEncObj->Base.State == ObjState_Running) {
                pAudEncObj->Base.State = ObjState_Stopped;

                pAudEncObj->TotalSubEvents.ModuleStopEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pAudEncObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizOriAudFrmCaptured: {
            if (pAudEncObj->Base.State == ObjState_Running) {
                // 1: post the BizAudStrmBitsEncodedEvent
                IOC_EvtDesc_T EvtDesc = {
                    .EvtID = IOC_EVTID_BizAudStrmBitsEncoded,
                };
                Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

                // 2: updateStatistics
                pAudEncObj->TotalSubEvents.BizOriAudFrmCapturedEvent++;    // subEvt: BizOriAudFrmCapturedEvent
                pAudEncObj->TotalPostEvents.BizAudStrmBitsEncodedEvent++;  // postEvt: BizAudStrmBitsEncodedEvent

                // 3: tell ModMgr Iam alive
                __postKeepAliveEvt(&pAudEncObj->Base);
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pAudEncObj->Base.State;
            }
        } break;

        default: {
            // BUG: unexpected event
            EXPECT_FALSE(true) << "BUG: unexpected event " << EvtID;
        } break;
    }

    return Result;
}

static void __Case01_setupAudEncObj(_LiveCamAudEncObj_pT pAudEncObj) {
    // SETUP ZERO AS DEFAULT
    memset(pAudEncObj, 0, sizeof(_LiveCamAudEncObj_T));

    pAudEncObj->Base.ObjID             = ObjID_AudEncObj;
    pAudEncObj->Base.State             = ObjState_Stopped;
    pAudEncObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_ModStart,
        IOC_EVTID_ModStop,
        IOC_EVTID_BizOriAudFrmCaptured,
    };
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_AudEncObj,
        .pCbPrivData = pAudEncObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupAudEncObj(_LiveCamAudEncObj_pT pAudEncObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_AudEncObj,
        .pCbPrivData = pAudEncObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "AudEncObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_verifyAudEncObj(_LiveCamAudEncObj_pT pAudEncObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, pAudEncObj->Base.State);
    EXPECT_EQ(_CASE01_DURATION, pAudEncObj->Base.KeepAliveCnt);
    EXPECT_EQ(_CASE01_AUDCAP_FRM_CNT, pAudEncObj->TotalSubEvents.BizOriAudFrmCapturedEvent);
    EXPECT_EQ(_CASE01_AUDCAP_FRM_CNT, pAudEncObj->TotalPostEvents.BizAudStrmBitsEncodedEvent);

    EXPECT_EQ(1, pAudEncObj->TotalSubEvents.ModuleStartEvent);
    EXPECT_EQ(1, pAudEncObj->TotalSubEvents.ModuleStopEvent);
}

static IOC_Result_T __Case01_cbProcEvt_VidResizeObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamVidResizeObj_pT VidResizeObj = (_LiveCamVidResizeObj_pT)pCbPriv;
    IOC_Result_T Result                  = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID                    = pEvtDesc->EvtID;

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    switch (EvtID) {
        case IOC_EVTID_ModStart: {
            if (VidResizeObj->Base.State == ObjState_Stopped) {
                VidResizeObj->Base.State = ObjState_Running;

                VidResizeObj->TotalSubEvents.ModuleStartEvent++;
                Result                   = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect stopped state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect stopped state, but " << VidResizeObj->Base.State;
            }
        } break;

        case IOC_EVTID_ModStop: {
            if (VidResizeObj->Base.State == ObjState_Running) {
                VidResizeObj->Base.State = ObjState_Stopped;

                VidResizeObj->TotalSubEvents.ModuleStopEvent++;
                Result                   = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << VidResizeObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizOriVidFrmCaptured: {
            if (VidResizeObj->Base.State == ObjState_Running) {
                // 1: post the BizLoResVidFrmResizedEvent and BizOriVidFrmRecycledEvent
                IOC_EvtDesc_T EvtDesc = {
                    .EvtID = IOC_EVTID_BizLoResVidFrmResized,
                };
                Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

                EvtDesc.EvtID = IOC_EVTID_BizOriVidFrmRecycled;
                Result        = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

                // 2: updateStatistics
                VidResizeObj->TotalSubEvents.BizOriVidFrmCapturedEvent++;  // procEvt: BizOriVidFrmCaptured

                VidResizeObj->TotalPostEvents.BizLoResVidFrmResizedEvent++;  // postEvt: BizLoResVidFrmResized
                VidResizeObj->TotalPostEvents.BizOriVidFrmRecycledEvent++;   // postEvt: BizOriVidFrmRecycled

                // 3: tell ModMgr Iam alive
                __postKeepAliveEvt(&VidResizeObj->Base);
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << VidResizeObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizLoResVidFrmRecycled: {
            if (VidResizeObj->Base.State == ObjState_Running) {
                VidResizeObj->TotalSubEvents.BizLoResVidFrmRecycledEvent++;  // procEvt: BizLoResVidFrmRecycled

                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << VidResizeObj->Base.State;
            }
        } break;

        default: {
            // BUG: unexpected event
            EXPECT_FALSE(true) << "BUG: unexpected event " << IOC_getEvtClassID(EvtID) << ":" << IOC_getEvtNameID(EvtID);
        } break;
    }

    return Result;
}

static void __Case01_setupVidResizeObj(_LiveCamVidResizeObj_pT VidResizeObj) {
    // SETUP ZERO AS DEFAULT
    memset(VidResizeObj, 0, sizeof(_LiveCamVidResizeObj_T));

    VidResizeObj->Base.ObjID             = ObjID_VidResizeObj;
    VidResizeObj->Base.State             = ObjState_Stopped;
    VidResizeObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_ModStart,
        IOC_EVTID_ModStop,
        IOC_EVTID_BizOriVidFrmCaptured,
        IOC_EVTID_BizLoResVidFrmRecycled,
    };
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_VidResizeObj,
        .pCbPrivData = VidResizeObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupVidResizeObj(_LiveCamVidResizeObj_pT VidResizeObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_VidResizeObj,
        .pCbPrivData = VidResizeObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "VidResizeObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_verifyVidResizeObj(_LiveCamVidResizeObj_pT VidResizeObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, VidResizeObj->Base.State);
    EXPECT_EQ(_CASE01_DURATION, VidResizeObj->Base.KeepAliveCnt);

    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, VidResizeObj->TotalPostEvents.BizLoResVidFrmResizedEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, VidResizeObj->TotalPostEvents.BizOriVidFrmRecycledEvent);

    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, VidResizeObj->TotalSubEvents.BizOriVidFrmCapturedEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, VidResizeObj->TotalSubEvents.BizLoResVidFrmRecycledEvent);

    EXPECT_EQ(1, VidResizeObj->TotalSubEvents.ModuleStartEvent);
    EXPECT_EQ(1, VidResizeObj->TotalSubEvents.ModuleStopEvent);
}

static IOC_Result_T __Case01_cbProcEvt_LoResVidEncObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamLoResVidEncObj_pT LoResVidEncObj = (_LiveCamLoResVidEncObj_pT)pCbPriv;
    IOC_Result_T Result                      = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID                        = pEvtDesc->EvtID;

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    switch (EvtID) {
        case IOC_EVTID_ModStart: {
            if (LoResVidEncObj->Base.State == ObjState_Stopped) {
                LoResVidEncObj->Base.State = ObjState_Running;
                LoResVidEncObj->TotalSubEvents.ModuleStartEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect stopped state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect stopped state, but " << LoResVidEncObj->Base.State;
            }
        } break;

        case IOC_EVTID_ModStop: {
            if (LoResVidEncObj->Base.State == ObjState_Running) {
                LoResVidEncObj->Base.State = ObjState_Stopped;
                LoResVidEncObj->TotalSubEvents.ModuleStopEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << LoResVidEncObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizLoResVidFrmResized: {
            if (LoResVidEncObj->Base.State == ObjState_Running) {
                // 1: post the BizLoResVidStrmBitsEncodedEvent and BizLoResVidFrmRecycledEvent
                IOC_EvtDesc_T EvtDesc = {
                    .EvtID = IOC_EVTID_BizLoResVidStrmBitsEncoded,
                };
                Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

                EvtDesc.EvtID = IOC_EVTID_BizLoResVidFrmRecycled;
                Result        = IOC_postEVT_inConlesMode(&EvtDesc, NULL);

                // 2: updateStatistics
                LoResVidEncObj->TotalSubEvents.BizLoResVidFrmResizedEvent++;  // procEvt: BizLoResVidFrmResized

                LoResVidEncObj->TotalPostEvents
                    .BizLoResVidStrmBitsEncodedEvent++;                         // postEvt: BizLoResVidStrmBitsEncoded
                LoResVidEncObj->TotalPostEvents.BizLoResVidFrmRecycledEvent++;  // postEvt: BizLoResVidFrmRecycled

                // 3: tell ModMgr Iam alive
                __postKeepAliveEvt(&LoResVidEncObj->Base);

                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << LoResVidEncObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizLoResVidStrmBitsRecycled: {
            if (LoResVidEncObj->Base.State == ObjState_Running) {
                LoResVidEncObj->TotalSubEvents
                    .BizLoResVidStrmBitsRecycledEvent++;  // procEvt: BizLoResVidStrmBitsRecycled
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << LoResVidEncObj->Base.State;
            }
        } break;

        default: {
            // BUG: unexpected event
            EXPECT_FALSE(true) << "BUG: unexpected event " << EvtID;
        } break;
    }

    return Result;
}

static void __Case01_setupLoResVidEncObj(_LiveCamLoResVidEncObj_pT LoResVidEncObj) {
    // SETUP ZERO AS DEFAULT
    memset(LoResVidEncObj, 0, sizeof(_LiveCamLoResVidEncObj_T));

    LoResVidEncObj->Base.ObjID             = ObjID_LoResVidEncObj;
    LoResVidEncObj->Base.State             = ObjState_Stopped;
    LoResVidEncObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_ModStart,
        IOC_EVTID_ModStop,
        IOC_EVTID_BizLoResVidFrmResized,
        IOC_EVTID_BizLoResVidStrmBitsRecycled,
    };
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_LoResVidEncObj,
        .pCbPrivData = LoResVidEncObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupLoResVidEncObj(_LiveCamLoResVidEncObj_pT LoResVidEncObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_LoResVidEncObj,
        .pCbPrivData = LoResVidEncObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "LoResVidEncObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_verifyLoResVidEncObj(_LiveCamLoResVidEncObj_pT LoResVidEncObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, LoResVidEncObj->Base.State);
    EXPECT_EQ(_CASE01_DURATION, LoResVidEncObj->Base.KeepAliveCnt);

    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, LoResVidEncObj->TotalPostEvents.BizLoResVidStrmBitsEncodedEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, LoResVidEncObj->TotalPostEvents.BizLoResVidFrmRecycledEvent);

    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, LoResVidEncObj->TotalSubEvents.BizLoResVidFrmResizedEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, LoResVidEncObj->TotalSubEvents.BizLoResVidStrmBitsRecycledEvent);

    EXPECT_EQ(1, LoResVidEncObj->TotalSubEvents.ModuleStartEvent);
    EXPECT_EQ(1, LoResVidEncObj->TotalSubEvents.ModuleStopEvent);
}

static IOC_Result_T __Case01_cbProcEvt_LoResStrmMuxObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamLoResStrmMuxObj_pT pLoResStrmMuxObj = (_LiveCamLoResStrmMuxObj_T *)pCbPriv;
    IOC_Result_T Result                         = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID                           = pEvtDesc->EvtID;
    IOC_EvtDesc_T EvtDesc                       = {};

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    switch (EvtID) {
        case IOC_EVTID_ModStart: {
            if (pLoResStrmMuxObj->Base.State == ObjState_Stopped) {
                pLoResStrmMuxObj->Base.State = ObjState_Running;
                pLoResStrmMuxObj->TotalSubEvents.ModuleStartEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect stopped state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect stopped state, but " << pLoResStrmMuxObj->Base.State;
            }
        } break;

        case IOC_EVTID_ModStop: {
            if (pLoResStrmMuxObj->Base.State == ObjState_Running) {
                pLoResStrmMuxObj->Base.State = ObjState_Stopped;
                pLoResStrmMuxObj->TotalSubEvents.ModuleStopEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pLoResStrmMuxObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizLoResVidStrmBitsEncoded: {
            if (pLoResStrmMuxObj->Base.State == ObjState_Running) {
                // 1 VidStrmBits post BizLoResStrmBitsMuxedEvent and BizLoResStrmBitsRecycledEvent

                EvtDesc.EvtID = IOC_EVTID_BizLoResStrmBitsMuxed;
                Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

                // 2 update statistics
                pLoResStrmMuxObj->TotalSubEvents
                    .BizLoResVidStrmBitsEncodedEvent++;  // procEvt: BizLoResVidStrmBitsEncoded

                pLoResStrmMuxObj->TotalPostEvents.BizLoResStrmBitsMuxedEvent++;  // postEvt: BizLoResStrmBitsMuxed

                // 3 tell ModMgr Iam alive
                __postKeepAliveEvt(&pLoResStrmMuxObj->Base);

                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pLoResStrmMuxObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizAudStrmBitsEncoded: {
            if (pLoResStrmMuxObj->Base.State == ObjState_Running) {
                // AudStrmBits update statistics only
                pLoResStrmMuxObj->TotalSubEvents.BizAudStrmBitsEncodedEvent++;  // procEvt: BizAudStrmBitsEncoded
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pLoResStrmMuxObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizLoResStrmBitsRecycled: {
            if (pLoResStrmMuxObj->Base.State == ObjState_Running) {
                pLoResStrmMuxObj->TotalSubEvents.BizLoResStrmBitsRecycledEvent++;

                // relay LoResStrmBitsRecycled -> LoResVidStrmBitsRecycled
                EvtDesc.EvtID = IOC_EVTID_BizLoResVidStrmBitsRecycled;
                Result        = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

                pLoResStrmMuxObj->TotalPostEvents
                    .BizLoResVidStrmBitsRecycledEvent++;  // postEvt: BizLoResVidStrmBitsRecycled

                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pLoResStrmMuxObj->Base.State;
            }
        } break;

        default: {
            // BUG: unexpected event
            EXPECT_FALSE(true) << "BUG: unexpected event " << EvtID;
        } break;
    }

    return Result;
}

static void __Case01_setupLoResStrmMuxObj(_LiveCamLoResStrmMuxObj_pT pLoResStrmMuxObj) {
    // SETUP ZERO AS DEFAULT
    memset(pLoResStrmMuxObj, 0, sizeof(_LiveCamLoResStrmMuxObj_T));

    pLoResStrmMuxObj->Base.ObjID             = ObjID_LoResStrmMuxObj;
    pLoResStrmMuxObj->Base.State             = ObjState_Stopped;
    pLoResStrmMuxObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_ModStart,
        IOC_EVTID_ModStop,
        IOC_EVTID_BizLoResVidStrmBitsEncoded,
        IOC_EVTID_BizAudStrmBitsEncoded,
        IOC_EVTID_BizLoResStrmBitsRecycled,
    };
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_LoResStrmMuxObj,
        .pCbPrivData = pLoResStrmMuxObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupLoResStrmMuxObj(_LiveCamLoResStrmMuxObj_pT pLoResStrmMuxObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_LoResStrmMuxObj,
        .pCbPrivData = pLoResStrmMuxObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "LoResStrmMuxObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_verifyLoResStrmMuxObj(_LiveCamLoResStrmMuxObj_pT pLoResStrmMuxObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, pLoResStrmMuxObj->Base.State);
    EXPECT_EQ(_CASE01_DURATION, pLoResStrmMuxObj->Base.KeepAliveCnt);

    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pLoResStrmMuxObj->TotalPostEvents.BizLoResStrmBitsMuxedEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pLoResStrmMuxObj->TotalPostEvents.BizLoResVidStrmBitsRecycledEvent);

    EXPECT_EQ(_CASE01_AUDCAP_FRM_CNT, pLoResStrmMuxObj->TotalSubEvents.BizAudStrmBitsEncodedEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pLoResStrmMuxObj->TotalSubEvents.BizLoResStrmBitsRecycledEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pLoResStrmMuxObj->TotalSubEvents.BizLoResVidStrmBitsEncodedEvent);

    EXPECT_EQ(1, pLoResStrmMuxObj->TotalSubEvents.ModuleStartEvent);
    EXPECT_EQ(1, pLoResStrmMuxObj->TotalSubEvents.ModuleStopEvent);
}

static IOC_Result_T __Case01_cbProcEvt_SrvObj(IOC_EvtDesc_pT pEvtDesc, void *pCbPriv) {
    _LiveCamSrvObj_pT pSrvObj = (_LiveCamSrvObj_pT)pCbPriv;
    IOC_Result_T Result       = IOC_RESULT_BUG;
    IOC_EvtID_T EvtID         = pEvtDesc->EvtID;

    __Case01_verifyFromPostEvt2CbProcEvtPerfMeets(pEvtDesc);

    switch (EvtID) {
        case IOC_EVTID_ModStart: {
            if (pSrvObj->Base.State == ObjState_Stopped) {
                pSrvObj->Base.State = ObjState_Running;
                pSrvObj->TotalSubEvents.ModuleStartEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect stopped state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect stopped state, but " << pSrvObj->Base.State;
            }
        } break;

        case IOC_EVTID_ModStop: {
            if (pSrvObj->Base.State == ObjState_Running) {
                pSrvObj->Base.State = ObjState_Stopped;
                pSrvObj->TotalSubEvents.ModuleStopEvent++;
                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pSrvObj->Base.State;
            }
        } break;

        case IOC_EVTID_BizLoResStrmBitsMuxed: {
            if (pSrvObj->Base.State == ObjState_Running) {
                // 1: post the BizLoResStrmBitsSentEvent and BizLoResStrmBitsRecycledEvent
                IOC_EvtDesc_T EvtDesc = {
                    .EvtID    = IOC_EVTID_BizLoResStrmBitsSent,
                    .EvtValue = pEvtDesc->EvtValue,
                };
                Result = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                // Success or NoEvtSuber
                EXPECT_TRUE(Result == IOC_RESULT_SUCCESS || Result == IOC_RESULT_NO_EVENT_CONSUMER);

                EvtDesc.EvtID = IOC_EVTID_BizLoResStrmBitsRecycled;
                Result        = IOC_postEVT_inConlesMode(&EvtDesc, NULL);
                EXPECT_EQ(IOC_RESULT_SUCCESS, Result);

                // 2: updateStatistics
                pSrvObj->TotalSubEvents.BizLoResStrmBitsMuxedEvent++;  // procEvt: BizLoResStrmBitsMuxed

                pSrvObj->TotalPostEvents.BizLoResStrmBitsSentEvent++;      // postEvt: BizLoResStrmBitsSent
                pSrvObj->TotalPostEvents.BizLoResStrmBitsRecycledEvent++;  // postEvt: BizLoResStrmBitsRecycled

                // 3: tell ModMgr Iam alive
                __postKeepAliveEvt(&pSrvObj->Base);

                Result = IOC_RESULT_SUCCESS;
            } else {
                // BUG: expect running state, but ??? state
                EXPECT_FALSE(true) << "BUG: expect running state, but " << pSrvObj->Base.State;
            }
        } break;

        default: {
            // BUG: unexpected event
            EXPECT_FALSE(true) << "BUG: unexpected event " << EvtID;
        } break;
    }

    return Result;
}

static void __Case01_setupSrvObj(_LiveCamSrvObj_pT pSrvObj) {
    // SETUP ZERO AS DEFAULT
    memset(pSrvObj, 0, sizeof(_LiveCamSrvObj_T));

    pSrvObj->Base.ObjID             = ObjID_SrvObj;
    pSrvObj->Base.State             = ObjState_Stopped;
    pSrvObj->Base.LastKeepAliveTime = IOC_getCurrentTimeSpec();

    IOC_EvtID_T SubEvtIDs[] = {
        IOC_EVTID_ModStart,      IOC_EVTID_ModStop,        IOC_EVTID_BizLoResStrmBitsMuxed,
        IOC_EVTID_SrvOpenStream, IOC_EVTID_SrvCloseStream,
    };
    IOC_SubEvtArgs_T SubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_SrvObj,
        .pCbPrivData = pSrvObj,

        .EvtNum  = IOC_calcArrayElmtCnt(SubEvtIDs),
        .pEvtIDs = SubEvtIDs,
    };

    IOC_Result_T Result = IOC_subEVT_inConlesMode(&SubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result);
}

static void __Case01_cleanupSrvObj(_LiveCamSrvObj_pT pSrvObj) {
    // CLEANUP
    IOC_UnsubEvtArgs_T UnsubEvtArgs = {
        .CbProcEvt_F = __Case01_cbProcEvt_SrvObj,
        .pCbPrivData = pSrvObj,
    };

    IOC_Result_T Result = IOC_unsubEVT_inConlesMode(&UnsubEvtArgs);
    EXPECT_EQ(IOC_RESULT_SUCCESS, Result) << "SrvObj unsubEVT fail, Result=" << IOC_getResultStr(Result);
}

static void __Case01_verifySrvObj(_LiveCamSrvObj_pT pSrvObj) {
    // VERIFY
    EXPECT_EQ(ObjState_Stopped, pSrvObj->Base.State);
    EXPECT_EQ(_CASE01_DURATION, pSrvObj->Base.KeepAliveCnt);

    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pSrvObj->TotalPostEvents.BizLoResStrmBitsSentEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pSrvObj->TotalPostEvents.BizLoResStrmBitsRecycledEvent);
    EXPECT_EQ(_CASE01_VIDCAP_FRM_CNT, pSrvObj->TotalSubEvents.BizLoResStrmBitsMuxedEvent);

    EXPECT_EQ(1, pSrvObj->TotalSubEvents.ModuleStartEvent);
    EXPECT_EQ(1, pSrvObj->TotalSubEvents.ModuleStopEvent);
}

TEST(UT_ConlesEventDemoLiveCam, verifyFunctionality_v0_1_0) {
    // SETUP
    _LiveCamVidCapObj_T VidCapObj;
    __Case01_setupVidCapObj(&VidCapObj);

    _LiveCamAudCapObj_T AudCapObj;
    __Case01_setupAudCapObj(&AudCapObj);

    _LiveCamAudEncObj_T AudEncObj;
    __Case01_setupAudEncObj(&AudEncObj);

    _LiveCamVidResizeObj_T VidResizeObj;
    __Case01_setupVidResizeObj(&VidResizeObj);

    _LiveCamLoResVidEncObj_T LoResVidEncObj;
    __Case01_setupLoResVidEncObj(&LoResVidEncObj);

    _LiveCamLoResStrmMuxObj_T LoResStrmMuxObj;
    __Case01_setupLoResStrmMuxObj(&LoResStrmMuxObj);

    _LiveCamSrvObj_T SrvObj;
    __Case01_setupSrvObj(&SrvObj);

    _LiveCamModMgrObj_T ModMgrObj;
    __Case01_setupModMgrObj(&ModMgrObj);

    //[TODO@W UseCase::Category-B]: _LiveCamCliObjFactory_T CliObjFactory;

    // BEHAVIOR
    __Case01_startModMgrObj(&ModMgrObj);
    //[TODO@W UseCase::Category-B]: __startCliObjFactory(&CliObjFactory);

    sleep(_CASE01_DURATION + (_CASE01_DURATION / 10) /*10%*/);  // wait a little bit more time,
    // becaused all FPS is sleep-NNms based and may not be accurate as expected

    __Case01_stopModMgrObj(&ModMgrObj);
    //[TODO@W UseCase::Category-B]: __stopCliObjFactory(&CliObjFactory);

    // VERIFY
    // force all pending events to be processed
    IOC_forceProcEVT();

    __Case01_verifyVidCapObj(&VidCapObj);
    __Case01_verifyAudCapObj(&AudCapObj);
    __Case01_verifyAudEncObj(&AudEncObj);
    __Case01_verifyVidResizeObj(&VidResizeObj);
    __Case01_verifyLoResVidEncObj(&LoResVidEncObj);
    __Case01_verifyLoResStrmMuxObj(&LoResStrmMuxObj);
    __Case01_verifySrvObj(&SrvObj);
    __Case01_verifyModMgrObj(&ModMgrObj);

    //[TODO@W UseCase::Category-B]: __verifyCliObjFactory(&CliObjFactory);

    // CLEANUP
    __Case01_cleanupVidCapObj(&VidCapObj);
    __Case01_cleanupAudCapObj(&AudCapObj);
    __Case01_cleanupAudEncObj(&AudEncObj);
    __Case01_cleanupVidResizeObj(&VidResizeObj);
    __Case01_cleanupLoResVidEncObj(&LoResVidEncObj);
    __Case01_cleanupLoResStrmMuxObj(&LoResStrmMuxObj);
    __Case01_cleanupSrvObj(&SrvObj);
    __Case01_cleanupModMgrObj(&ModMgrObj);

    //[TODO@W UseCase::Category-B]: __cleanupCliObjFactory(&CliObjFactory);
}

/**
 * @brief UT to verify perforamce of ConlesEventDemoLiveCam.
 *    First class performance is delay time from postEVT to cbProcEvt, so
 *      - update UT_ConlesEventDemoLiveCamCase01.verifyFunctionality_v0_1_0 to measure the delay time of each event.
 *
 */
// TEST(UT_ConlesEventDemoLiveCamCase02, verifyPerformance) {}

// TEST(UT_ConlesEventDemoLiveCamCase03, verifyRobustness) {}

//======>>>>>>END OF UT IMPLEMENTATION<<<<<<<===========================================================================