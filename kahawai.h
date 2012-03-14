//Master renders hi quality, slave renders low quality
enum KAHAWAI_MODE { Master=0, Slave=1};
#define MAPPING_SIZE (1024*768*3)+18

extern KAHAWAI_MODE kahawaiProcessId;
extern HANDLE kahawaiMutex;
extern HANDLE kahawaiMap;
extern HANDLE kahawaiSlaveBarrier;
extern HANDLE kahawaiMasterBarrier;