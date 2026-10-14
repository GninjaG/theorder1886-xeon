import hashlib
import base64
from pathlib import Path

def nid(name):
    salt = bytes.fromhex('518d64a635ded8c1e6b039b1c3e55230')
    digest = hashlib.sha1(name.encode('ascii') + salt).digest()[:8][::-1]
    return base64.b64encode(digest).decode().rstrip('=').replace('/', '-')

unknown_nids = {
    "T8fER+tIGgk": "libScePosix",
    "P41kTWUS3EI": "libkernel",
    "oIRFTjoILbg": "libkernel",
    "Xs9hdiD7sAA": "libScePosix",
    "BpFoboUJoZU": "libkernel",
    "8mql9OcQnd4": "libkernel",
    "JTvBflhYazQ": "libkernel",
    "9lvj5DjHZiA": "libkernel",
    "7uhBFWRAS60": "libkernel",
    "IOnSvHzqu6A": "libkernel",
    "fgIsQ10xYVA": "libkernel",
    "0Cq8ipKr9n0": "libkernel",
    "j2AIqSqJP0w": "libkernel",
    "FXPWHNk8Of0": "libkernel",
    "m5-2bsNfv7s": "libkernel",
    "waPcxYiR3WA": "libkernel",
    "0-KXaS70xy4": "libScePosix",
    "WrOLvHU0yQM": "libScePosix",
    "mqULNdimTn0": "libScePosix",
    "6BpEZuDT7YI": "libScePosix",
    "Fc8qxlKINYQ": "libSceVideoRecording",
    "tIYf0W5VTi8": "libSceScreenShot",
    "2xxUtuC-RzE": "libSceScreenShot",
    "3rHWaV-1KC4": "libSceAppContent",
    "6H-71OdrpXM": "libSceMsgDialog",
    "v6EZ-YWRdMs": "libScePlayGo",
    "-Q1-u1a7p0g": "libScePlayGo",
    "MTxxrOCeSig": "libSceVideoOut",
    "-4GCfYdNF1s": "libSceIme",
    "eaFXjfJv3xs": "libSceIme",
    "dKadqZFgKKQ": "libSceIme",
    "PMVehSlfZ94": "libSceIme",
    "XTWR0UXvcgs": "libSceAppContent",
    "VANhIWcqYak": "libSceAppContent",
    "NLq2b1jOaN0": "libkernel"
}

# Candidate function names to test
candidates = [
    # libSceIme
    "sceImeOpen", "sceImeClose", "sceImeUpdate", "sceImeSetText", "sceImeGetText", "sceImeInit",
    "sceImeKeyboardOpen", "sceImeKeyboardClose", "sceImeKeyboardUpdate", "sceImeKeyboardGetEvent",
    "sceImeDialogInit", "sceImeDialogGetResult", "sceImeDialogGetStatus", "sceImeDialogTerm", "sceImeDialogOpen",
    "sceImeDialogClose", "sceImeDialogAbort", "sceImeGetParam", "sceImeSetParam",
    
    # libSceScreenShot
    "sceScreenShotSetOverlayImage", "sceScreenShotDisable", "sceScreenShotEnable", "sceScreenShotCapture",
    "sceScreenShotIsDisabled",
    
    # libSceVideoRecording
    "sceVideoRecordingOpen", "sceVideoRecordingClose", "sceVideoRecordingStart", "sceVideoRecordingStop",
    "sceVideoRecordingSetProhibition", "sceVideoRecordingSetProhibitionNotification",
    
    # libSceAppContent
    "sceAppContentAddcontMount", "sceAppContentAddcontUnmount", "sceAppContentAddcontGetMountPath",
    "sceAppContentTemporaryDataMount", "sceAppContentTemporaryDataUnmount", "sceAppContentTemporaryDataFormat",
    "sceAppContentDownloadDataMount", "sceAppContentDownloadDataUnmount", "sceAppContentDownloadDataFormat",
    "sceAppContentGetEntitlementKey", "sceAppContentAppParamGetString", "sceAppContentGetAddcontInfoList",
    
    # libScePlayGo
    "scePlayGoGetChunkStatus", "scePlayGoGetTodoChunk", "scePlayGoPrefetch", "scePlayGoGetEta",
    
    # libSceVideoOut
    "sceVideoOutGetResolutionStatus", "sceVideoOutSetColorimetry", "sceVideoOutGetPortType",
    "sceVideoOutClose", "sceVideoOutGetBufferAttribute", "sceVideoOutSetBufferAttribute2",
    "sceVideoOutGetResolutionStatus2",
    
    # libSceMsgDialog
    "sceMsgDialogGetResult", "sceMsgDialogGetStatus", "sceMsgDialogAbort", "sceMsgDialogClose",
    
    # posix / kernel
    "pthread_mutex_init", "pthread_mutex_destroy", "pthread_mutex_lock", "pthread_mutex_unlock",
    "pthread_cond_init", "pthread_cond_destroy", "pthread_cond_wait", "pthread_cond_signal",
    "pthread_cond_broadcast", "pthread_create", "pthread_join", "pthread_detach",
    "pthread_setname_np", "pthread_getname_np", "pthread_equal", "pthread_self",
    "sched_yield", "usleep", "sleep", "nanosleep", "clock_gettime", "clock_getres",
    "gettimeofday", "sysconf", "getpid", "getpagesize",
    "mmap", "munmap", "mprotect", "msync", "mlock", "munlock",
    "read", "write", "open", "close", "lseek", "stat", "fstat", "access",
    "mkdir", "rmdir", "unlink", "rename", "chmod", "fchmod",
    "socket", "connect", "bind", "listen", "accept", "send", "recv", "sendto", "recvfrom",
    "setsockopt", "getsockopt", "select", "poll", "ioctl", "fcntl", "pipe", "dup", "dup2",
    "truncate", "ftruncate", "fsync", "fdatasync", "getuid", "geteuid", "getgid", "getegid",
    "sysctl", "sysctlbyname", "sysctlbyname_np", "sigaction", "sigprocmask",
    "pthread_rwlock_init", "pthread_rwlock_destroy", "pthread_rwlock_rdlock", "pthread_rwlock_wrlock", "pthread_rwlock_unlock",
    "pthread_attr_init", "pthread_attr_destroy", "pthread_attr_setstacksize", "pthread_attr_getstacksize",
    "pthread_attr_setschedparam", "pthread_attr_getschedparam", "pthread_attr_setinheritsched",
    "pthread_barrier_init", "pthread_barrier_destroy", "pthread_barrier_wait",
    
    # sceKernel
    "sceKernelCreateSema", "sceKernelDeleteSema", "sceKernelWaitSema", "sceKernelPollSema", "sceKernelSignalSema",
    "sceKernelCreateEventFlag", "sceKernelDeleteEventFlag", "sceKernelSetEventFlag", "sceKernelClearEventFlag", "sceKernelWaitEventFlag", "sceKernelPollEventFlag",
    "sceKernelAllocateDirectMemory", "sceKernelMapDirectMemory", "sceKernelReleaseDirectMemory", "sceKernelGetDirectMemorySize",
    "sceKernelGetProcessTime", "sceKernelGetProcessTimeCounter", "sceKernelGetProcessTimeCounterFrequency",
    "sceKernelUsleep", "sceKernelNanosleep", "sceKernelSleep",
    "sceKernelGetCurrentCpu", "sceKernelGetIdps", "sceKernelGetOpenPsId", "sceKernelSendNotificationRequest",
    "sceKernelGetTscFrequency", "sceKernelChmod", "sceKernelMkdir", "sceKernelRmdir", "sceKernelUnlink",
    "sceKernelRename", "sceKernelOpen", "sceKernelClose", "sceKernelRead", "sceKernelWrite", "sceKernelLseek",
    "sceKernelStat", "sceKernelFstat", "sceKernelGetdents", "sceKernelBatchMap", "sceKernelMapNamedDirectMemory",
    "sceKernelGetDirectMemoryType", "sceKernelIccSetBuzzer", "sceKernelGetCpuTemperature"
]

matched = {}
for c in candidates:
    n = nid(c)
    if n in unknown_nids:
        matched[n] = (c, unknown_nids[n])

print(f"Matched {len(matched)} / {len(unknown_nids)} unknown NIDs:")
for n, (name, lib) in matched.items():
    print(f"  {n}#{lib} -> {name}")
