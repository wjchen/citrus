#include "citrus/err.hpp"
#include "internal.hpp"

#include <string.h>

#include <sstream>
#include <unordered_map>

namespace ctr {
    namespace err {
        static std::unordered_map<u32, const std::string> sourceStrings = {
                {SOURCE_GENERIC, "SOURCE_GENERIC"},
                {SOURCE_PROCESS_CLOSING, "SOURCE_PROCESS_CLOSING"},
                {SOURCE_OPERATION_CANCELLED, "SOURCE_OPERATION_CANCELLED"},

                {SOURCE_APP_INIT, "SOURCE_APP_INIT"},
                {SOURCE_APP_IO_ERROR, "SOURCE_APP_IO_ERROR"},
                {SOURCE_APP_BEGIN_INSTALL, "SOURCE_APP_BEGIN_INSTALL"},
                {SOURCE_APP_WRITE_CIA, "SOURCE_APP_WRITE_CIA"},
                {SOURCE_APP_FINALIZE_INSTALL, "SOURCE_APP_FINALIZE_INSTALL"},
                {SOURCE_APP_DELETE_TITLE, "SOURCE_APP_DELETE_TITLE"},
                {SOURCE_APP_PREPARE_LAUNCH, "SOURCE_APP_PREPARE_LAUNCH"},
                {SOURCE_APP_DO_LAUNCH, "SOURCE_APP_DO_LAUNCH"},
                {SOURCE_APP_GET_TITLE_COUNT, "SOURCE_APP_GET_TITLE_COUNT"},
                {SOURCE_APP_GET_TITLE_ID_LIST, "SOURCE_APP_GET_TITLE_ID_LIST"},
                {SOURCE_APP_GET_TITLE_INFO, "SOURCE_APP_GET_TITLE_INFO"},
                {SOURCE_APP_OPEN_ARCHIVE, "SOURCE_APP_OPEN_ARCHIVE"},
                {SOURCE_APP_OPEN_FILE, "SOURCE_APP_OPEN_FILE"},
                {SOURCE_APP_GET_DEVICE_ID, "SOURCE_APP_GET_DEVICE_ID"},

                {SOURCE_BATTERY_INIT, "SOURCE_BATTERY_INIT"},
                {SOURCE_BATTERY_GET_CHARGE_STATE, "SOURCE_BATTERY_GET_CHARGE_STATE"},
                {SOURCE_BATTERY_GET_LEVEL, "SOURCE_BATTERY_GET_LEVEL"},

                {SOURCE_FS_GET_NAND_RESOURCE, "SOURCE_FS_GET_NAND_RESOURCE"},
                {SOURCE_FS_GET_SD_RESOURCE, "SOURCE_FS_GET_SD_RESOURCE"},

                {SOURCE_IR_ALLOCATE_BUFFER, "SOURCE_IR_ALLOCATE_BUFFER"},
                {SOURCE_IR_INIT, "SOURCE_IR_INIT"},
                {SOURCE_IR_GET_STATE, "SOURCE_IR_GET_STATE"},
                {SOURCE_IR_SET_STATE, "SOURCE_IR_SET_STATE"},

                {SOURCE_NEWS_INIT, "SOURCE_NEWS_INIT"},
                {SOURCE_NEWS_ADD_NOTIFICATION, "SOURCE_NEWS_ADD_NOTIFICATION"},

                {SOURCE_NOR_INIT, "SOURCE_NOR_INIT"},
                {SOURCE_NOR_READ_DATA, "SOURCE_NOR_READ_DATA"},
                {SOURCE_NOR_WRITE_DATA, "SOURCE_NOR_WRITE_DATA"},

                {SOURCE_SND_INIT, "SOURCE_SND_INIT"},
                {SOURCE_SND_INVALID_CHANNEL, "SOURCE_SND_INVALID_CHANNEL"},
                {SOURCE_SND_EXEC_CMDS, "SOURCE_SND_EXEC_CMDS"},

                {SOURCE_SOC_ALLOCATE_BUFFER, "SOURCE_SOC_ALLOCATE_BUFFER"},
                {SOURCE_SOC_INIT, "SOURCE_SOC_INIT"},

                {SOURCE_WIFI_INIT, "SOURCE_WIFI_INIT"},
                {SOURCE_WIFI_GET_STATUS, "SOURCE_WIFI_GET_STATUS"},
                {SOURCE_WIFI_WAIT_INTERNET_CONNECTION, "SOURCE_WIFI_WAIT_INTERNET_CONNECTION"}
        };

        static std::unordered_map<u32, const std::string> moduleStrings = {
                {MODULE_COMMON, "MODULE_COMMON"},
                {MODULE_NN_KERNEL, "MODULE_NN_KERNEL"},
                {MODULE_NN_UTIL, "MODULE_NN_UTIL"},
                {MODULE_NN_FILE_SERVER, "MODULE_NN_FILE_SERVER"},
                {MODULE_NN_LOADER_SERVER, "MODULE_NN_LOADER_SERVER"},
                {MODULE_NN_TCB, "MODULE_NN_TCB"},
                {MODULE_NN_OS, "MODULE_NN_OS"},
                {MODULE_NN_DBG, "MODULE_NN_DBG"},
                {MODULE_NN_DMNT, "MODULE_NN_DMNT"},
                {MODULE_NN_PDN, "MODULE_NN_PDN"},
                {MODULE_NN_GX, "MODULE_NN_GX"},
                {MODULE_NN_I2C, "MODULE_NN_I2C"},
                {MODULE_NN_GPIO, "MODULE_NN_GPIO"},
                {MODULE_NN_DD, "MODULE_NN_DD"},
                {MODULE_NN_CODEC, "MODULE_NN_CODEC"},
                {MODULE_NN_SPI, "MODULE_NN_SPI"},
                {MODULE_NN_PXI, "MODULE_NN_PXI"},
                {MODULE_NN_FS, "MODULE_NN_FS"},
                {MODULE_NN_DI, "MODULE_NN_DI"},
                {MODULE_NN_HID, "MODULE_NN_HID"},
                {MODULE_NN_CAMERA, "MODULE_NN_CAMERA"},
                {MODULE_NN_PI, "MODULE_NN_PI"},
                {MODULE_NN_PM, "MODULE_NN_PM"},
                {MODULE_NN_PMLOW, "MODULE_NN_PMLOW"},
                {MODULE_NN_FSI, "MODULE_NN_FSI"},
                {MODULE_NN_SRV, "MODULE_NN_SRV"},
                {MODULE_NN_NDM, "MODULE_NN_NDM"},
                {MODULE_NN_NWM, "MODULE_NN_NWM"},
                {MODULE_NN_SOCKET, "MODULE_NN_SOCKET"},
                {MODULE_NN_LDR, "MODULE_NN_LDR"},
                {MODULE_NN_ACC, "MODULE_NN_ACC"},
                {MODULE_NN_ROMFS, "MODULE_NN_ROMFS"},
                {MODULE_NN_AM, "MODULE_NN_AM"},
                {MODULE_NN_HIO, "MODULE_NN_HIO"},
                {MODULE_NN_UPDATER, "MODULE_NN_UPDATER"},
                {MODULE_NN_MIC, "MODULE_NN_MIC"},
                {MODULE_NN_FND, "MODULE_NN_FND"},
                {MODULE_NN_MP, "MODULE_NN_MP"},
                {MODULE_NN_MPWL, "MODULE_NN_MPWL"},
                {MODULE_NN_AC, "MODULE_NN_AC"},
                {MODULE_NN_HTTP, "MODULE_NN_HTTP"},
                {MODULE_NN_DSP, "MODULE_NN_DSP"},
                {MODULE_NN_SND, "MODULE_NN_SND"},
                {MODULE_NN_DLP, "MODULE_NN_DLP"},
                {MODULE_NN_HIOLOW, "MODULE_NN_HIOLOW"},
                {MODULE_NN_CSND, "MODULE_NN_CSND"},
                {MODULE_NN_SSL, "MODULE_NN_SSL"},
                {MODULE_NN_AMLOW, "MODULE_NN_AMLOW"},
                {MODULE_NN_NEX, "MODULE_NN_NEX"},
                {MODULE_NN_FRIENDS, "MODULE_NN_FRIENDS"},
                {MODULE_NN_RDT, "MODULE_NN_RDT"},
                {MODULE_NN_APPLET, "MODULE_NN_APPLET"},
                {MODULE_NN_NIM, "MODULE_NN_NIM"},
                {MODULE_NN_PTM, "MODULE_NN_PTM"},
                {MODULE_NN_MIDI, "MODULE_NN_MIDI"},
                {MODULE_NN_MC, "MODULE_NN_MC"},
                {MODULE_NN_SWC, "MODULE_NN_SWC"},
                {MODULE_NN_FATFS, "MODULE_NN_FATFS"},
                {MODULE_NN_NGC, "MODULE_NN_NGC"},
                {MODULE_NN_CARD, "MODULE_NN_CARD"},
                {MODULE_NN_CARDNOR, "MODULE_NN_CARDNOR"},
                {MODULE_NN_SDMC, "MODULE_NN_SDMC"},
                {MODULE_NN_BOSS, "MODULE_NN_BOSS"},
                {MODULE_NN_DBM, "MODULE_NN_DBM"},
                {MODULE_NN_CONFIG, "MODULE_NN_CONFIG"},
                {MODULE_NN_PS, "MODULE_NN_PS"},
                {MODULE_NN_CEC, "MODULE_NN_CEC"},
                {MODULE_NN_IR, "MODULE_NN_IR"},
                {MODULE_NN_UDS, "MODULE_NN_UDS"},
                {MODULE_NN_PL, "MODULE_NN_PL"},
                {MODULE_NN_CUP, "MODULE_NN_CUP"},
                {MODULE_NN_GYROSCOPE, "MODULE_NN_GYROSCOPE"},
                {MODULE_NN_MCU, "MODULE_NN_MCU"},
                {MODULE_NN_NS, "MODULE_NN_NS"},
                {MODULE_NN_NEWS, "MODULE_NN_NEWS"},
                {MODULE_NN_RO, "MODULE_NN_RO"},
                {MODULE_NN_GD, "MODULE_NN_GD"},
                {MODULE_NN_CARDSPI, "MODULE_NN_CARDSPI"},
                {MODULE_NN_EC, "MODULE_NN_EC"},
                {MODULE_NN_WEBBRS, "MODULE_NN_WEBBRS"},
                {MODULE_NN_TEST, "MODULE_NN_TEST"},
                {MODULE_NN_ENC, "MODULE_NN_ENC"},
                {MODULE_NN_PIA, "MODULE_NN_PIA"},
                {MODULE_NN_MVD, "MODULE_NN_MVD"},
                {MODULE_NN_QTM, "MODULE_NN_QTM"},
                {MODULE_APPLICATION, "MODULE_APPLICATION"},
                {MODULE_INVALID_RESULT_VALUE, "MODULE_INVALID_RESULT_VALUE"}
        };

        static std::unordered_map<u32, const std::string> levelStrings = {
                {LEVEL_SUCCESS, "LEVEL_SUCCESS"},
                {LEVEL_INFO, "LEVEL_INFO"},
                {LEVEL_STATUS, "LEVEL_STATUS"},
                {LEVEL_TEMPORARY, "LEVEL_TEMPORARY"},
                {LEVEL_PERMANENT, "LEVEL_PERMANENT"},
                {LEVEL_USAGE, "LEVEL_USAGE"},
                {LEVEL_REINIT, "LEVEL_REINIT"},
                {LEVEL_RESET, "LEVEL_RESET"},
                {LEVEL_FATAL, "LEVEL_FATAL"}
        };

        static std::unordered_map<u32, const std::string> summaryStrings = {
                {SUMMARY_SUCCESS, "SUMMARY_SUCCESS"},
                {SUMMARY_NOTHING_HAPPENED, "SUMMARY_NOTHING_HAPPENED"},
                {SUMMARY_WOULD_BLOCK, "SUMMARY_WOULD_BLOCK"},
                {SUMMARY_OUT_OF_RESOURCE, "SUMMARY_OUT_OF_RESOURCE"},
                {SUMMARY_NOT_FOUND, "SUMMARY_NOT_FOUND"},
                {SUMMARY_INVALID_STATE, "SUMMARY_INVALID_STATE"},
                {SUMMARY_NOT_SUPPORTED, "SUMMARY_NOT_SUPPORTED"},
                {SUMMARY_INVALID_ARGUMENT, "SUMMARY_INVALID_ARGUMENT"},
                {SUMMARY_WRONG_ARGUMENT, "SUMMARY_WRONG_ARGUMENT"},
                {SUMMARY_CANCELED, "SUMMARY_CANCELED"},
                {SUMMARY_STATUS_CHANGED, "SUMMARY_STATUS_CHANGED"},
                {SUMMARY_INTERNAL, "SUMMARY_INTERNAL"},
                {SUMMARY_INVALID_RESULT_VALUE, "SUMMARY_INVALID_RESULT_VALUE"}
        };

        static std::unordered_map<u32, const std::string> descriptionStrings = {
                {DESCRIPTION_SUCCESS, "DESCRIPTION_SUCCESS"},
                {DESCRIPTION_INVALID_MEMORY_PERMISSIONS, "DESCRIPTION_INVALID_MEMORY_PERMISSIONS"},
                {DESCRIPTION_INVALID_TICKET_VERSION, "DESCRIPTION_INVALID_TICKET_VERSION"},
                {DESCRIPTION_STRING_TOO_BIG, "DESCRIPTION_STRING_TOO_BIG"},
                {DESCRIPTION_ACCESS_DENIED, "DESCRIPTION_ACCESS_DENIED"},
                {DESCRIPTION_STRING_TOO_SMALL, "DESCRIPTION_STRING_TOO_SMALL"},
                {DESCRIPTION_CAMERA_BUSY, "DESCRIPTION_CAMERA_BUSY"},
                {DESCRIPTION_NOT_ENOUGH_MEMORY, "DESCRIPTION_NOT_ENOUGH_MEMORY"},
                {DESCRIPTION_SESSION_CLOSED_BY_REMOTE, "DESCRIPTION_SESSION_CLOSED_BY_REMOTE"},
                {DESCRIPTION_INVALID_NCCH, "DESCRIPTION_INVALID_NCCH"},
                {DESCRIPTION_INVALID_TITLE_VERSION, "DESCRIPTION_INVALID_TITLE_VERSION"},
                {DESCRIPTION_DATABASE_DOES_NOT_EXIST, "DESCRIPTION_DATABASE_DOES_NOT_EXIST"},
                {DESCRIPTION_TRIED_TO_UNINSTALL_SYSTEM_APP, "DESCRIPTION_TRIED_TO_UNINSTALL_SYSTEM_APP"},
                {DESCRIPTION_ARCHIVE_NOT_MOUNTED, "DESCRIPTION_ARCHIVE_NOT_MOUNTED"},
                {DESCRIPTION_REQUEST_TIMED_OUT, "DESCRIPTION_REQUEST_TIMED_OUT"},
                {DESCRIPTION_INVALID_SIGNATURE, "DESCRIPTION_INVALID_SIGNATURE"},
                {DESCRIPTION_TITLE_NOT_FOUND, "DESCRIPTION_TITLE_NOT_FOUND"},
                {DESCRIPTION_GAMECARD_NOT_INSERTED, "DESCRIPTION_GAMECARD_NOT_INSERTED"},
                {DESCRIPTION_INVALID_FILE_OPEN_FLAGS, "DESCRIPTION_INVALID_FILE_OPEN_FLAGS"},
                {DESCRIPTION_INVALID_CONFIGURATION, "DESCRIPTION_INVALID_CONFIGURATION"},
                {DESCRIPTION_NCCH_HASH_CHECK_FAILED, "DESCRIPTION_NCCH_HASH_CHECK_FAILED"},
                {DESCRIPTION_AES_VERIFICATION_FAILED, "DESCRIPTION_AES_VERIFICATION_FAILED"},
                {DESCRIPTION_INVALID_DATABASE, "DESCRIPTION_INVALID_DATABASE"},
                {DESCRIPTION_SAVE_HASH_CHECK_FAILED, "DESCRIPTION_SAVE_HASH_CHECK_FAILED"},
                {DESCRIPTION_COMMAND_PERMISSION_DENIED, "DESCRIPTION_COMMAND_PERMISSION_DENIED"},
                {DESCRIPTION_INVALID_PATH, "DESCRIPTION_INVALID_PATH"},
                {DESCRIPTION_INCORRECT_READ_SIZE, "DESCRIPTION_INCORRECT_READ_SIZE"},
                {DESCRIPTION_INVALID_SELECTION, "DESCRIPTION_INVALID_SELECTION"},
                {DESCRIPTION_TOO_LARGE, "DESCRIPTION_TOO_LARGE"},
                {DESCRIPTION_NOT_AUTHORIZED, "DESCRIPTION_NOT_AUTHORIZED"},
                {DESCRIPTION_ALREADY_DONE, "DESCRIPTION_ALREADY_DONE"},
                {DESCRIPTION_INVALID_SIZE, "DESCRIPTION_INVALID_SIZE"},
                {DESCRIPTION_INVALID_ENUM_VALUE, "DESCRIPTION_INVALID_ENUM_VALUE"},
                {DESCRIPTION_INVALID_COMBINATION, "DESCRIPTION_INVALID_COMBINATION"},
                {DESCRIPTION_NO_DATA, "DESCRIPTION_NO_DATA"},
                {DESCRIPTION_BUSY, "DESCRIPTION_BUSY"},
                {DESCRIPTION_MISALIGNED_ADDRESS, "DESCRIPTION_MISALIGNED_ADDRESS"},
                {DESCRIPTION_MISALIGNED_SIZE, "DESCRIPTION_MISALIGNED_SIZE"},
                {DESCRIPTION_OUT_OF_MEMORY, "DESCRIPTION_OUT_OF_MEMORY"},
                {DESCRIPTION_NOT_IMPLEMENTED, "DESCRIPTION_NOT_IMPLEMENTED"},
                {DESCRIPTION_INVALID_ADDRESS, "DESCRIPTION_INVALID_ADDRESS"},
                {DESCRIPTION_INVALID_POINTER, "DESCRIPTION_INVALID_POINTER"},
                {DESCRIPTION_INVALID_HANDLE, "DESCRIPTION_INVALID_HANDLE"},
                {DESCRIPTION_NOT_INITIALIZED, "DESCRIPTION_NOT_INITIALIZED"},
                {DESCRIPTION_ALREADY_INITIALIZED, "DESCRIPTION_ALREADY_INITIALIZED"},
                {DESCRIPTION_NOT_FOUND, "DESCRIPTION_NOT_FOUND"},
                {DESCRIPTION_CANCEL_REQUESTED, "DESCRIPTION_CANCEL_REQUESTED"},
                {DESCRIPTION_ALREADY_EXISTS, "DESCRIPTION_ALREADY_EXISTS"},
                {DESCRIPTION_OUT_OF_RANGE, "DESCRIPTION_OUT_OF_RANGE"},
                {DESCRIPTION_TIMEOUT, "DESCRIPTION_TIMEOUT"},
                {DESCRIPTION_INVALID_RESULT_VALUE, "DESCRIPTION_INVALID_RESULT_VALUE"}
        };

        static Error currentError;
    }
}

bool ctr::err::init() {
    currentError = {SOURCE_GENERIC, MODULE_COMMON, LEVEL_SUCCESS, SUMMARY_SUCCESS, DESCRIPTION_SUCCESS};
    return true;
}

void ctr::err::exit() {
    currentError = {SOURCE_GENERIC, MODULE_COMMON, LEVEL_SUCCESS, SUMMARY_SUCCESS, DESCRIPTION_SUCCESS};
}

bool ctr::err::has() {
    return currentError.source != SOURCE_GENERIC || currentError.module != MODULE_COMMON || currentError.level != LEVEL_SUCCESS || currentError.summary != SUMMARY_SUCCESS || currentError.description != DESCRIPTION_SUCCESS;
}

ctr::err::Error ctr::err::get() {
    Error error = currentError;
    if(has()) {
        currentError = {SOURCE_GENERIC, MODULE_COMMON, LEVEL_SUCCESS, SUMMARY_SUCCESS, DESCRIPTION_SUCCESS};
    }

    return error;
}

void ctr::err::set(ctr::err::Error error) {
    currentError = error;
}

void ctr::err::parse(ctr::err::Source source, u32 raw) {
    Error err;

    #define GET_BITS(v, s, e) (((v) >> (s)) & ((1 << ((e) - (s) + 1)) - 1))
    err.source = source;
    err.module = (Module) GET_BITS(raw, 10, 17);
    err.level = (Level) GET_BITS(raw, 27, 31);
    err.summary = (Summary) GET_BITS(raw, 21, 26);
    err.description = (Description) GET_BITS(raw, 0, 9);
    #undef GET_BITS

    set(err);
}

const std::string ctr::err::toString(ctr::err::Error error) {
    std::stringstream result;

    std::unordered_map<u32, const std::string>::iterator sourceResult = sourceStrings.find(error.source);
    const std::string sourceString = sourceResult != sourceStrings.end() ? sourceResult->second : "<unknown>";

    result << "Source: " << sourceString << " (0x" << std::hex << error.source << ")" << "\n";

    if(error.source == SOURCE_APP_IO_ERROR) {
        result << "Error: " << strerror((int) error.description) << " (0x" << std::hex << (u32) error.description << ")" << "\n";
    } else {
        std::unordered_map<u32, const std::string>::iterator moduleResult = moduleStrings.find(error.module);
        std::unordered_map<u32, const std::string>::iterator levelResult = levelStrings.find(error.level);
        std::unordered_map<u32, const std::string>::iterator summaryResult = summaryStrings.find(error.summary);
        std::unordered_map<u32, const std::string>::iterator descriptionResult = descriptionStrings.find(error.description);

        const std::string moduleString = moduleResult != moduleStrings.end() ? moduleResult->second : "<unknown>";
        const std::string levelString = levelResult != levelStrings.end() ? levelResult->second : "<unknown>";
        const std::string summaryString = summaryResult != summaryStrings.end() ? summaryResult->second : "<unknown>";
        const std::string descriptionString = descriptionResult != descriptionStrings.end() ? descriptionResult->second : "<unknown>";

        result << "Module: " << moduleString << " (0x" << std::hex << error.module << ")" << "\n";
        result << "Level: " << levelString << " (0x" << std::hex << error.level << ")" << "\n";
        result << "Summary: " << summaryString << " (0x" << std::hex << error.summary << ")" << "\n";
        result << "Description: " << descriptionString << " (0x" << std::hex << error.description << ")";
    }

    return result.str();
}