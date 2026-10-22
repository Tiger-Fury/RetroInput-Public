// Execute the trampoline in this test process without patching or starting a game.
#include "../RetroInput/Games/TheSuffering/GameHooks.cpp"
#include <cassert>

alignas(16) static BYTE beforeState[512] = {};
alignas(16) static BYTE afterState[512] = {};
alignas(16) static const DWORD vectorValue[4] = {1, 2, 3, 4};
static DWORD record[8] = {0, 256, 0, 0, 0, 0x23456789, 0, 0x12345678};
static float input[256] = {};
static DWORD savedEax, savedEdx, savedEbp, savedEsi, savedEdi, savedEcx;
static bool called = false;

void __stdcall InjectControllerInput(DWORD deviceRecord, float* state)
{
    assert(deviceRecord == reinterpret_cast<DWORD>(record));
    assert(state == input);
    called = true;
    state[0] = 1.0f;
    __asm {
        fninit
        fldz
        xorps xmm0, xmm0
    }
}

__declspec(naked) void ExerciseHook()
{
    __asm {
        pushfd
        pushad
        fninit
        fld1
        fldpi
        movaps xmm0, vectorValue
        fxsave beforeState
        mov ebx, offset record
        mov esi, offset input
        mov ebp, 0ABCD1234h
        mov edi, 0DEAD5678h
        mov ecx, 0CAFE9876h
        mov g_ReturnAddress, offset ResumeTest
        jmp InputHook
    ResumeTest:
        mov savedEax, eax
        mov savedEdx, edx
        mov savedEbp, ebp
        mov savedEsi, esi
        mov savedEdi, edi
        mov savedEcx, ecx
        fxsave afterState
        fninit
        popad
        popfd
        ret
    }
}

int main()
{
    ExerciseHook();
    assert(called && input[0] == 1.0f);
    assert(savedEax == record[5] && savedEdx == record[7]);
    assert(savedEbp == 0xABCD1234 && savedEdi == 0xDEAD5678 && savedEcx == 0xCAFE9876);
    assert(savedEsi == reinterpret_cast<DWORD>(input));
    assert(std::memcmp(beforeState, afterState, 5) == 0); // x87 control/status/tag
    assert(std::memcmp(beforeState + 24, afterState + 24, 4) == 0); // MXCSR
    assert(std::memcmp(beforeState + 32, afterState + 32, 32) == 0); // two live x87 values
    assert(std::memcmp(beforeState + 160, afterState + 160, 16) == 0); // XMM0
}

