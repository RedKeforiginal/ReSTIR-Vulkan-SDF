#include <gflags/gflags.h>

#include <csignal>
#include <cstdio>
#include <ctime>
#include <exception>

#include "app.h"

DEFINE_bool(enable_validation, false, "Enable Vulkan validation layers and debug utils.");
DEFINE_bool(validate_assets, false, "Validate shader assets before startup.");

namespace {
FILE* g_logFile = nullptr;

void logMessage(const char* message) {
	std::fputs(message, stderr);
	std::fflush(stderr);
	if (g_logFile) {
		std::fputs(message, g_logFile);
		std::fflush(g_logFile);
	}
}

void logTimestamped(const char* message) {
	std::time_t now = std::time(nullptr);
	std::tm* local = std::localtime(&now);
	if (local) {
		char buffer[64];
		if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local) > 0) {
			std::fprintf(stderr, "[%s] %s", buffer, message);
			std::fflush(stderr);
			if (g_logFile) {
				std::fprintf(g_logFile, "[%s] %s", buffer, message);
				std::fflush(g_logFile);
			}
			return;
		}
	}
	logMessage(message);
}

void signalHandler(int signal) {
	switch (signal) {
	case SIGSEGV:
		logTimestamped("Fatal signal: SIGSEGV\n");
		break;
	case SIGABRT:
		logTimestamped("Fatal signal: SIGABRT\n");
		break;
	case SIGFPE:
		logTimestamped("Fatal signal: SIGFPE\n");
		break;
	case SIGILL:
		logTimestamped("Fatal signal: SIGILL\n");
		break;
	default:
		logTimestamped("Fatal signal: unknown\n");
		break;
	}
	std::_Exit(signal);
}

void terminateHandler() {
	try {
		throw;
	} catch (const std::exception& ex) {
		logTimestamped("Unhandled exception: ");
		logMessage(ex.what());
		logMessage("\n");
	} catch (...) {
		logTimestamped("Unhandled non-standard exception.\n");
	}
	std::_Exit(EXIT_FAILURE);
}

void setupCrashReporting() {
	g_logFile = std::fopen("startup.log", "a");
	logTimestamped("Starting application.\n");
	std::set_terminate(terminateHandler);
	std::signal(SIGSEGV, signalHandler);
	std::signal(SIGABRT, signalHandler);
	std::signal(SIGFPE, signalHandler);
	std::signal(SIGILL, signalHandler);
}
} // namespace

int main(int argc, char **argv) {
	setupCrashReporting();
	try {
		gflags::ParseCommandLineFlags(&argc, &argv, true);
		App app(FLAGS_enable_validation, FLAGS_validate_assets);
		app.mainLoop();
	} catch (const std::exception& ex) {
		logTimestamped("Unhandled exception in main: ");
		logMessage(ex.what());
		logMessage("\n");
		return EXIT_FAILURE;
	} catch (...) {
		logTimestamped("Unhandled non-standard exception in main.\n");
		return EXIT_FAILURE;
	}
	return 0;
}
