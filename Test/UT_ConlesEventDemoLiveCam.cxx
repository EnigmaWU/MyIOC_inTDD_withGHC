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
#include "_UT_IOC_Common.h"
// Define DemoLiveCam's Event Class use IOC_EVT_CLASS_TEST
#define IOC_EVT_CLASS_LIVECAM IOC_EVT_CLASS_TEST

// Define DemoLiveCam's Event Name by events between LiveCam's module objects.
typedef enum {
    // RefEvent flow of service side module objects from Biz Viewpoint
    IOC_ENAME_BizOriVidFrmCaptured,
    IOC_ENAME_BizOriVidFrmRecycled,
    IOC_ENAME_BizHiResVidStrmBitsEncoded,
    IOC_ENAME_BizHiResVidStrmBitsRecycled,
    IOC_ENAME_BizHiResStrmBitsMuxed,
    IOC_ENAME_BizHiResStrmBitsRecycled,

    IOC_ENAME_BizLoResVidFrmResized,
    IOC_ENAME_BizLoResVidFrmRecycled,
    IOC_ENAME_BizLoResVidStrmBitsEncoded,
    IOC_ENAME_BizLoResVidStrmBitsRecycled,
    IOC_ENAME_BizLoResStrmBitsMuxed,
    IOC_ENAME_BizLoResStrmBitsRecycled,

    IOC_ENAME_BizOriAudFrmCaptured,
    IOC_ENAME_BizAudStrmBitsEncoded,

    // RefEvent flow of service side module objects from Biz Viewpoint
    IOC_ENAME_ModStart,
    IOC_ENAME_ModStop,
    IOC_ENAME_ModKeepAlive,

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

typedef enum {
    ObjState_Stopped,  // stopped state
                       // initial state or stopped by user on IOC_EVTID_ModStop from running state
    ObjState_Running,  // running state
                       // started by user on IOC_EVTID_ModStart from stopped state
} _LiveCamObjState_T;

// RefBrief: server and client side module objects
typedef struct {
    _LiveCamObjState_T State;
    struct timespec LastKeepAliveTime;
} _LiveCamObjBase_T, *LiveCamObjBase_pT;

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

    // postEVT
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
    } TotalStartEvents;  // TotalMgntSpecEvents

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
    } TotalStopEvents;  // TotalMgntSpecEvents

} _LiveCamModMgrObj_T, *LiveCamModMgrObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;       // TotalMgntSpecEvents
    } TotalPostEvents;

} _LiveCamVidCapObj_T, *LiveCamVidCapObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;       // TotalMgntSpecEvents
    } TotalPostEvents;

} _LiveCamAudCapObj_T, *LiveCamAudCapObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;        // TotalMgntSpecEvents
    } TotalPostEvents;

} _LiveCamAudEncObj_T, *LiveCamAudEncObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;             // TotalMgntSpecEvents
        ULONG_T BizOriVidFrmRecycledEvent;        // TotalBizSpecEvents
    } TotalPostEvents;
} _LiveCamHiResVidEncObj_T, *LiveCamHiResVidEncObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;             // TotalMgntSpecEvents
        ULONG_T BizLoResVidFrmRecycledEvent;      // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamLoResVidEncObj_T, *LiveCamLoResVidEncObj_pT;

// RefBrief: VidResizeObj
typedef struct {
    _LiveCamObjBase_T Base;

    // subEVT
    struct {
        ULONG_T ModuleStartEvent;           // TotalMgntSpecEvents
        ULONG_T ModuleStopEvent;            // TotalMgntSpecEvents
        ULONG_T BizOriVidFrmCapturedEvent;  // TotalBizSpecEvents
    } TotalSubEvents;

    // postEVT
    struct {
        ULONG_T BizOriVidFrmRecycledEvent;   // TotalBizSpecEvents
        ULONG_T ModuleKeepAliveEvent;        // TotalMgntSpecEvents
        ULONG_T BizLoResVidFrmResizedEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamVidResizeObj_T, *LiveCamVidResizeObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;              // TotalMgntSpecEvents
        ULONG_T BizHiResVidStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamHiResStrmMuxObj_T, *LiveCamHiResStrmMuxObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;              // TotalMgntSpecEvents
        ULONG_T BizLoResVidStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamLoResStrmMuxObj_T, *LiveCamLoResStrmMuxObj_pT;

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
        ULONG_T ModuleKeepAliveEvent;           // TotalMgntSpecEvents
        ULONG_T BizHiResStrmBitsRecycledEvent;  // TotalBizSpecEvents
        ULONG_T BizLoResStrmBitsRecycledEvent;  // TotalBizSpecEvents
    } TotalPostEvents;

} _LiveCamSrvObj_T, *LiveCamSrvObj_pT;

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
 * @[Name]: UT_ConlesEventDemoLiveCam_verifyFunctionality_v0_1_0
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
 *      ViCapObj's EVTCNT of ModuleKeepAliveEvent is 100,
 *      AudCapObj's EVTCNT of ModuleKeepAliveEvent is 100.
 * @[Notes]: N/A
 */
TEST(UT_ConlesEventDemoLiveCam, verifyFunctionality_v0_1_0) {}

TEST(UT_ConlesEventDemoLiveCam, verifyPerformance) {}

TEST(UT_ConlesEventDemoLiveCam, verifyRobustness) {}

//======>>>>>>END OF UT IMPLEMENTATION<<<<<<<===========================================================================