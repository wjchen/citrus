#include "citrus/err.hpp"
#include "internal.hpp"

#include <sstream>
#include <unordered_map>

namespace ctr {
    namespace err {
        static std::unordered_map<u32, const std::string> moduleStrings = {
                {ctr::err::MODULE_COMMON, "MODULE_COMMON"},
                {ctr::err::MODULE_NN_KERNEL, "MODULE_NN_KERNEL"},
                {ctr::err::MODULE_NN_UTIL, "MODULE_NN_UTIL"},
                {ctr::err::MODULE_NN_FILE_SERVER, "MODULE_NN_FILE_SERVER"},
                {ctr::err::MODULE_NN_LOADER_SERVER, "MODULE_NN_LOADER_SERVER"},
                {ctr::err::MODULE_NN_TCB, "MODULE_NN_TCB"},
                {ctr::err::MODULE_NN_OS, "MODULE_NN_OS"},
                {ctr::err::MODULE_NN_DBG, "MODULE_NN_DBG"},
                {ctr::err::MODULE_NN_DMNT, "MODULE_NN_DMNT"},
                {ctr::err::MODULE_NN_PDN, "MODULE_NN_PDN"},
                {ctr::err::MODULE_NN_GX, "MODULE_NN_GX"},
                {ctr::err::MODULE_NN_I2C, "MODULE_NN_I2C"},
                {ctr::err::MODULE_NN_GPIO, "MODULE_NN_GPIO"},
                {ctr::err::MODULE_NN_DD, "MODULE_NN_DD"},
                {ctr::err::MODULE_NN_CODEC, "MODULE_NN_CODEC"},
                {ctr::err::MODULE_NN_SPI, "MODULE_NN_SPI"},
                {ctr::err::MODULE_NN_PXI, "MODULE_NN_PXI"},
                {ctr::err::MODULE_NN_FS, "MODULE_NN_FS"},
                {ctr::err::MODULE_NN_DI, "MODULE_NN_DI"},
                {ctr::err::MODULE_NN_HID, "MODULE_NN_HID"},
                {ctr::err::MODULE_NN_CAMERA, "MODULE_NN_CAMERA"},
                {ctr::err::MODULE_NN_PI, "MODULE_NN_PI"},
                {ctr::err::MODULE_NN_PM, "MODULE_NN_PM"},
                {ctr::err::MODULE_NN_PMLOW, "MODULE_NN_PMLOW"},
                {ctr::err::MODULE_NN_FSI, "MODULE_NN_FSI"},
                {ctr::err::MODULE_NN_SRV, "MODULE_NN_SRV"},
                {ctr::err::MODULE_NN_NDM, "MODULE_NN_NDM"},
                {ctr::err::MODULE_NN_NWM, "MODULE_NN_NWM"},
                {ctr::err::MODULE_NN_SOCKET, "MODULE_NN_SOCKET"},
                {ctr::err::MODULE_NN_LDR, "MODULE_NN_LDR"},
                {ctr::err::MODULE_NN_ACC, "MODULE_NN_ACC"},
                {ctr::err::MODULE_NN_ROMFS, "MODULE_NN_ROMFS"},
                {ctr::err::MODULE_NN_AM, "MODULE_NN_AM"},
                {ctr::err::MODULE_NN_HIO, "MODULE_NN_HIO"},
                {ctr::err::MODULE_NN_UPDATER, "MODULE_NN_UPDATER"},
                {ctr::err::MODULE_NN_MIC, "MODULE_NN_MIC"},
                {ctr::err::MODULE_NN_FND, "MODULE_NN_FND"},
                {ctr::err::MODULE_NN_MP, "MODULE_NN_MP"},
                {ctr::err::MODULE_NN_MPWL, "MODULE_NN_MPWL"},
                {ctr::err::MODULE_NN_AC, "MODULE_NN_AC"},
                {ctr::err::MODULE_NN_HTTP, "MODULE_NN_HTTP"},
                {ctr::err::MODULE_NN_DSP, "MODULE_NN_DSP"},
                {ctr::err::MODULE_NN_SND, "MODULE_NN_SND"},
                {ctr::err::MODULE_NN_DLP, "MODULE_NN_DLP"},
                {ctr::err::MODULE_NN_HIOLOW, "MODULE_NN_HIOLOW"},
                {ctr::err::MODULE_NN_CSND, "MODULE_NN_CSND"},
                {ctr::err::MODULE_NN_SSL, "MODULE_NN_SSL"},
                {ctr::err::MODULE_NN_AMLOW, "MODULE_NN_AMLOW"},
                {ctr::err::MODULE_NN_NEX, "MODULE_NN_NEX"},
                {ctr::err::MODULE_NN_FRIENDS, "MODULE_NN_FRIENDS"},
                {ctr::err::MODULE_NN_RDT, "MODULE_NN_RDT"},
                {ctr::err::MODULE_NN_APPLET, "MODULE_NN_APPLET"},
                {ctr::err::MODULE_NN_NIM, "MODULE_NN_NIM"},
                {ctr::err::MODULE_NN_PTM, "MODULE_NN_PTM"},
                {ctr::err::MODULE_NN_MIDI, "MODULE_NN_MIDI"},
                {ctr::err::MODULE_NN_MC, "MODULE_NN_MC"},
                {ctr::err::MODULE_NN_SWC, "MODULE_NN_SWC"},
                {ctr::err::MODULE_NN_FATFS, "MODULE_NN_FATFS"},
                {ctr::err::MODULE_NN_NGC, "MODULE_NN_NGC"},
                {ctr::err::MODULE_NN_CARD, "MODULE_NN_CARD"},
                {ctr::err::MODULE_NN_CARDNOR, "MODULE_NN_CARDNOR"},
                {ctr::err::MODULE_NN_SDMC, "MODULE_NN_SDMC"},
                {ctr::err::MODULE_NN_BOSS, "MODULE_NN_BOSS"},
                {ctr::err::MODULE_NN_DBM, "MODULE_NN_DBM"},
                {ctr::err::MODULE_NN_CONFIG, "MODULE_NN_CONFIG"},
                {ctr::err::MODULE_NN_PS, "MODULE_NN_PS"},
                {ctr::err::MODULE_NN_CEC, "MODULE_NN_CEC"},
                {ctr::err::MODULE_NN_IR, "MODULE_NN_IR"},
                {ctr::err::MODULE_NN_UDS, "MODULE_NN_UDS"},
                {ctr::err::MODULE_NN_PL, "MODULE_NN_PL"},
                {ctr::err::MODULE_NN_CUP, "MODULE_NN_CUP"},
                {ctr::err::MODULE_NN_GYROSCOPE, "MODULE_NN_GYROSCOPE"},
                {ctr::err::MODULE_NN_MCU, "MODULE_NN_MCU"},
                {ctr::err::MODULE_NN_NS, "MODULE_NN_NS"},
                {ctr::err::MODULE_NN_NEWS, "MODULE_NN_NEWS"},
                {ctr::err::MODULE_NN_RO, "MODULE_NN_RO"},
                {ctr::err::MODULE_NN_GD, "MODULE_NN_GD"},
                {ctr::err::MODULE_NN_CARDSPI, "MODULE_NN_CARDSPI"},
                {ctr::err::MODULE_NN_EC, "MODULE_NN_EC"},
                {ctr::err::MODULE_NN_WEBBRS, "MODULE_NN_WEBBRS"},
                {ctr::err::MODULE_NN_TEST, "MODULE_NN_TEST"},
                {ctr::err::MODULE_NN_ENC, "MODULE_NN_ENC"},
                {ctr::err::MODULE_NN_PIA, "MODULE_NN_PIA"},
                {ctr::err::MODULE_NN_MVD, "MODULE_NN_MVD"},
                {ctr::err::MODULE_NN_QTM, "MODULE_NN_QTM"},
                {ctr::err::MODULE_APPLICATION, "MODULE_APPLICATION"},
                {ctr::err::MODULE_INVALID_RESULT_VALUE, "MODULE_INVALID_RESULT_VALUE"}
        };

        static std::unordered_map<u32, const std::string> levelStrings = {
                {ctr::err::LEVEL_SUCCESS, "LEVEL_SUCCESS"},
                {ctr::err::LEVEL_INFO, "LEVEL_INFO"},
                {ctr::err::LEVEL_STATUS, "LEVEL_STATUS"},
                {ctr::err::LEVEL_TEMPORARY, "LEVEL_TEMPORARY"},
                {ctr::err::LEVEL_PERMANENT, "LEVEL_PERMANENT"},
                {ctr::err::LEVEL_USAGE, "LEVEL_USAGE"},
                {ctr::err::LEVEL_REINIT, "LEVEL_REINIT"},
                {ctr::err::LEVEL_RESET, "LEVEL_RESET"},
                {ctr::err::LEVEL_FATAL, "LEVEL_FATAL"}
        };

        static std::unordered_map<u32, const std::string> summaryStrings = {
                {ctr::err::SUMMARY_SUCCESS, "SUMMARY_SUCCESS"},
                {ctr::err::SUMMARY_NOTHING_HAPPENED, "SUMMARY_NOTHING_HAPPENED"},
                {ctr::err::SUMMARY_WOULD_BLOCK, "SUMMARY_WOULD_BLOCK"},
                {ctr::err::SUMMARY_OUT_OF_RESOURCE, "SUMMARY_OUT_OF_RESOURCE"},
                {ctr::err::SUMMARY_NOT_FOUND, "SUMMARY_NOT_FOUND"},
                {ctr::err::SUMMARY_INVALID_STATE, "SUMMARY_INVALID_STATE"},
                {ctr::err::SUMMARY_NOT_SUPPORTED, "SUMMARY_NOT_SUPPORTED"},
                {ctr::err::SUMMARY_INVALID_ARGUMENT, "SUMMARY_INVALID_ARGUMENT"},
                {ctr::err::SUMMARY_WRONG_ARGUMENT, "SUMMARY_WRONG_ARGUMENT"},
                {ctr::err::SUMMARY_CANCELED, "SUMMARY_CANCELED"},
                {ctr::err::SUMMARY_STATUS_CHANGED, "SUMMARY_STATUS_CHANGED"},
                {ctr::err::SUMMARY_INTERNAL, "SUMMARY_INTERNAL"},
                {ctr::err::SUMMARY_INVALID_RESULT_VALUE, "SUMMARY_INVALID_RESULT_VALUE"}
        };

        static std::unordered_map<u32, const std::string> descriptionStrings = {
                {ctr::err::DESCRIPTION_SUCCESS, "DESCRIPTION_SUCCESS"},
                {ctr::err::DESCRIPTION_INVALID_MEMORY_PERMISSIONS, "DESCRIPTION_INVALID_MEMORY_PERMISSIONS"},
                {ctr::err::DESCRIPTION_INVALID_TICKET_VERSION, "DESCRIPTION_INVALID_TICKET_VERSION"},
                {ctr::err::DESCRIPTION_STRING_TOO_BIG, "DESCRIPTION_STRING_TOO_BIG"},
                {ctr::err::DESCRIPTION_ACCESS_DENIED, "DESCRIPTION_ACCESS_DENIED"},
                {ctr::err::DESCRIPTION_STRING_TOO_SMALL, "DESCRIPTION_STRING_TOO_SMALL"},
                {ctr::err::DESCRIPTION_CAMERA_BUSY, "DESCRIPTION_CAMERA_BUSY"},
                {ctr::err::DESCRIPTION_NOT_ENOUGH_MEMORY, "DESCRIPTION_NOT_ENOUGH_MEMORY"},
                {ctr::err::DESCRIPTION_SESSION_CLOSED_BY_REMOTE, "DESCRIPTION_SESSION_CLOSED_BY_REMOTE"},
                {ctr::err::DESCRIPTION_INVALID_NCCH, "DESCRIPTION_INVALID_NCCH"},
                {ctr::err::DESCRIPTION_INVALID_TITLE_VERSION, "DESCRIPTION_INVALID_TITLE_VERSION"},
                {ctr::err::DESCRIPTION_DATABASE_DOES_NOT_EXIST, "DESCRIPTION_DATABASE_DOES_NOT_EXIST"},
                {ctr::err::DESCRIPTION_TRIED_TO_UNINSTALL_SYSTEM_APP, "DESCRIPTION_TRIED_TO_UNINSTALL_SYSTEM_APP"},
                {ctr::err::DESCRIPTION_ARCHIVE_NOT_MOUNTED, "DESCRIPTION_ARCHIVE_NOT_MOUNTED"},
                {ctr::err::DESCRIPTION_REQUEST_TIMED_OUT, "DESCRIPTION_REQUEST_TIMED_OUT"},
                {ctr::err::DESCRIPTION_INVALID_SIGNATURE, "DESCRIPTION_INVALID_SIGNATURE"},
                {ctr::err::DESCRIPTION_TITLE_NOT_FOUND, "DESCRIPTION_TITLE_NOT_FOUND"},
                {ctr::err::DESCRIPTION_GAMECARD_NOT_INSERTED, "DESCRIPTION_GAMECARD_NOT_INSERTED"},
                {ctr::err::DESCRIPTION_INVALID_FILE_OPEN_FLAGS, "DESCRIPTION_INVALID_FILE_OPEN_FLAGS"},
                {ctr::err::DESCRIPTION_INVALID_CONFIGURATION, "DESCRIPTION_INVALID_CONFIGURATION"},
                {ctr::err::DESCRIPTION_NCCH_HASH_CHECK_FAILED, "DESCRIPTION_NCCH_HASH_CHECK_FAILED"},
                {ctr::err::DESCRIPTION_AES_VERIFICATION_FAILED, "DESCRIPTION_AES_VERIFICATION_FAILED"},
                {ctr::err::DESCRIPTION_INVALID_DATABASE, "DESCRIPTION_INVALID_DATABASE"},
                {ctr::err::DESCRIPTION_SAVE_HASH_CHECK_FAILED, "DESCRIPTION_SAVE_HASH_CHECK_FAILED"},
                {ctr::err::DESCRIPTION_COMMAND_PERMISSION_DENIED, "DESCRIPTION_COMMAND_PERMISSION_DENIED"},
                {ctr::err::DESCRIPTION_INVALID_PATH, "DESCRIPTION_INVALID_PATH"},
                {ctr::err::DESCRIPTION_INCORRECT_READ_SIZE, "DESCRIPTION_INCORRECT_READ_SIZE"},
                {ctr::err::DESCRIPTION_INVALID_SELECTION, "DESCRIPTION_INVALID_SELECTION"},
                {ctr::err::DESCRIPTION_TOO_LARGE, "DESCRIPTION_TOO_LARGE"},
                {ctr::err::DESCRIPTION_NOT_AUTHORIZED, "DESCRIPTION_NOT_AUTHORIZED"},
                {ctr::err::DESCRIPTION_ALREADY_DONE, "DESCRIPTION_ALREADY_DONE"},
                {ctr::err::DESCRIPTION_INVALID_SIZE, "DESCRIPTION_INVALID_SIZE"},
                {ctr::err::DESCRIPTION_INVALID_ENUM_VALUE, "DESCRIPTION_INVALID_ENUM_VALUE"},
                {ctr::err::DESCRIPTION_INVALID_COMBINATION, "DESCRIPTION_INVALID_COMBINATION"},
                {ctr::err::DESCRIPTION_NO_DATA, "DESCRIPTION_NO_DATA"},
                {ctr::err::DESCRIPTION_BUSY, "DESCRIPTION_BUSY"},
                {ctr::err::DESCRIPTION_MISALIGNED_ADDRESS, "DESCRIPTION_MISALIGNED_ADDRESS"},
                {ctr::err::DESCRIPTION_MISALIGNED_SIZE, "DESCRIPTION_MISALIGNED_SIZE"},
                {ctr::err::DESCRIPTION_OUT_OF_MEMORY, "DESCRIPTION_OUT_OF_MEMORY"},
                {ctr::err::DESCRIPTION_NOT_IMPLEMENTED, "DESCRIPTION_NOT_IMPLEMENTED"},
                {ctr::err::DESCRIPTION_INVALID_ADDRESS, "DESCRIPTION_INVALID_ADDRESS"},
                {ctr::err::DESCRIPTION_INVALID_POINTER, "DESCRIPTION_INVALID_POINTER"},
                {ctr::err::DESCRIPTION_INVALID_HANDLE, "DESCRIPTION_INVALID_HANDLE"},
                {ctr::err::DESCRIPTION_NOT_INITIALIZED, "DESCRIPTION_NOT_INITIALIZED"},
                {ctr::err::DESCRIPTION_ALREADY_INITIALIZED, "DESCRIPTION_ALREADY_INITIALIZED"},
                {ctr::err::DESCRIPTION_NOT_FOUND, "DESCRIPTION_NOT_FOUND"},
                {ctr::err::DESCRIPTION_CANCEL_REQUESTED, "DESCRIPTION_CANCEL_REQUESTED"},
                {ctr::err::DESCRIPTION_ALREADY_EXISTS, "DESCRIPTION_ALREADY_EXISTS"},
                {ctr::err::DESCRIPTION_OUT_OF_RANGE, "DESCRIPTION_OUT_OF_RANGE"},
                {ctr::err::DESCRIPTION_TIMEOUT, "DESCRIPTION_TIMEOUT"},
                {ctr::err::DESCRIPTION_INVALID_RESULT_VALUE, "DESCRIPTION_INVALID_RESULT_VALUE"}
        };

        static ctr::err::Error currentError;
    }
}

bool ctr::err::init() {
    currentError = {MODULE_COMMON, LEVEL_SUCCESS, SUMMARY_SUCCESS, DESCRIPTION_SUCCESS};
    return true;
}

void ctr::err::exit() {
    currentError = {MODULE_COMMON, LEVEL_SUCCESS, SUMMARY_SUCCESS, DESCRIPTION_SUCCESS};
}

bool ctr::err::has() {
    return currentError.module != MODULE_COMMON || currentError.level != LEVEL_SUCCESS || currentError.summary != SUMMARY_SUCCESS || currentError.description != DESCRIPTION_SUCCESS;
}

ctr::err::Error ctr::err::get() {
    Error error = currentError;
    if(has()) {
        currentError = {MODULE_COMMON, LEVEL_SUCCESS, SUMMARY_SUCCESS, DESCRIPTION_SUCCESS};
    }

    return error;
}

void ctr::err::set(ctr::err::Error error) {
    currentError = error;
}

void ctr::err::parse(u32 raw) {
    Error err;

    #define GET_BITS(v, s, e) (((v) >> (s)) & ((1 << ((e) - (s) + 1)) - 1))
    err.module = (Module) GET_BITS(raw, 10, 17);
    err.level = (Level) GET_BITS(raw, 27, 31);
    err.summary = (Summary) GET_BITS(raw, 21, 26);
    err.description = (Description) GET_BITS(raw, 0, 9);
    #undef GET_BITS

    set(err);
}

const std::string ctr::err::toString(ctr::err::Error error) {
    std::unordered_map<u32, const std::string>::iterator moduleResult = moduleStrings.find(error.module);
    std::unordered_map<u32, const std::string>::iterator levelResult = levelStrings.find(error.level);
    std::unordered_map<u32, const std::string>::iterator summaryResult = summaryStrings.find(error.summary);
    std::unordered_map<u32, const std::string>::iterator descriptionResult = descriptionStrings.find(error.description);

    const std::string moduleString = moduleResult != moduleStrings.end() ? moduleResult->second : "<unknown>";
    const std::string levelString = levelResult != levelStrings.end() ? levelResult->second : "<unknown>";
    const std::string summaryString = summaryResult != summaryStrings.end() ? summaryResult->second : "<unknown>";
    const std::string descriptionString = descriptionResult != descriptionStrings.end() ? descriptionResult->second : "<unknown>";

    std::stringstream result;
    result << "Module: " << moduleString << " (0x" << std::hex << error.module << ")" << "\n";
    result << "Level: " << levelString << " (0x" << std::hex << error.level << ")" << "\n";
    result << "Summary: " << summaryString << " (0x" << std::hex << error.summary << ")" << "\n";
    result << "Description: " << descriptionString << " (0x" << std::hex << error.description << ")";

    return result.str();
}