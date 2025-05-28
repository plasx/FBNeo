#define MAX_GUNS	4

struct BurnDialINF {
	INT32 VelocityStart;
	INT32 VelocityMax;
	INT32 VelocityMidpoint;
	INT32 Velocity;
	INT32 Backward;
	INT32 Forward;
};

extern INT32 nBurnGunNumPlayers;
extern bool bBurnGunHide[MAX_GUNS];
extern bool bBurnGunAutoHide;

extern INT32 BurnGunX[MAX_GUNS];
extern INT32 BurnGunY[MAX_GUNS];

#ifdef __cplusplus
extern "C" {
#endif
UINT8 BurnGunReturnX(INT32 num);
UINT8 BurnGunReturnY(INT32 num);
INT32 BurnGunInit(INT32 nNumPlayers, INT32 bDrawTargets);
void BurnGunExit();
void BurnGunScan();
void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y);
void BurnGunMakeInputs(INT32 num, INT16 x, INT16 y);
void BurnGunDrawTargets();
INT32 BurnGunIsActive();
void BurnGunSetBox(INT32 num, INT32 xmin, INT32 xmax, INT32 ymin, INT32 ymax);
void BurnGunSetCoords(INT32 player, INT32 x, INT32 y);

// NOTE: *depreciated* BurnPaddle is now the lowlevel code for BurnTrackball!
// Using BurnPaddle gives you 2 paddles (A & B) per player initted.
// BurnPaddleReturn[A/B] returns Velocity and directional data.
void BurnPaddleReturn(BurnDialINF &dial, INT32 num, INT32 isB);
void BurnPaddleGetDial(BurnDialINF &dial, INT32 num, INT32 isB);
void BurnPaddleSetWrap(INT32 num, INT32 xmin, INT32 xmax, INT32 ymin, INT32 ymax);
void BurnPaddleMakeInputs(INT32 num, BurnDialINF &dial, INT32 x, INT32 y);
#define BurnPaddleInit BurnTrackballInit
#define BurnPaddleExit BurnGunExit
#define BurnPaddleScan BurnGunScan

// Trackball Device (extension of BurnPaddle)
// see d_millipede.cpp, d_cabal or d_tempest.cpp for hook-up examples

void BurnTrackballInit(INT32 nNumPlayers);

#define AXIS_NORMAL 0
#define AXIS_REVERSED 1

// BurnTrackballFrame() is called at the beginning of a frame.  updates the analog positional status, etc.
void BurnTrackballFrame(INT32 dev, INT32 PortA, INT32 PortB, INT32 VelocityStart, INT32 VelocityMax, INT32 MaxScanlines = -1);

// BurnTrackballUpdate() is called once per frame, sometimes more to translate the velocity to movement (Tempest uses this)
void BurnTrackballUpdate(INT32 dev);
void BurnTrackballUpdatePortA(INT32 dev);
void BurnTrackballUpdatePortB(INT32 dev);
// slither (taito/d_qix.cpp) has a divider circuit on down + right signals
void BurnTrackballUpdateSlither(INT32 dev);

// BurnTrackballUDLR() can be used to load digital inputs into the trackball
void BurnTrackballUDLR(INT32 dev, INT32 u, INT32 d, INT32 l, INT32 r, INT32 speed = 0);

// Configure if an axis (Port) is reversed (1) or normal (0)
void BurnTrackballConfig(INT32 dev, INT32 PortA_rev, INT32 PortB_rev);
void BurnTrackballConfigStartStopPoints(INT32 dev, INT32 PortA_Start, INT32 PortA_Stop, INT32 PortB_Start, INT32 PortB_Stop);
void BurnTrackballSetVelocityCurve(INT32 bLogarithmic); // mostly for itech32

// Read the position counter
UINT8 BurnTrackballRead(INT32 dev, INT32 isB);
UINT16 BurnTrackballReadWord(INT32 dev, INT32 isB);
INT32 BurnTrackballReadSigned(INT32 dev, INT32 isB);
UINT8 BurnTrackballReadInterpolated(INT32 dev, INT32 isB, INT32 scanline);
INT32 BurnTrackballGetDirection(INT32 dev);
INT32 BurnTrackballGetVelocity(INT32 dev);
void BurnTrackballReadReset(INT32 dev, INT32 isB);

// Reset the position counters
void BurnTrackballReadResetAll(); // all devices
void BurnTrackballSetResetDefault(INT32 nDefault); // default is 0

#define BurnTrackballExit BurnGunExit
#define BurnTrackballScan BurnGunScan
#ifdef __cplusplus
}
#endif
