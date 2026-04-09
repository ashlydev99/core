//@ts-check
import { ENV_VAR_NAME } from "./const.js";

const cargoInstallCommand =
  "cargo install --git https://github.com/ashlydev99/core nexuschat-rpc-server";

export function NPM_NOT_FOUND_SUPPORTED_PLATFORM_ERROR(package_name) {
  return `nexuschat-rpc-server not found:

- Install it with "npm i ${package_name}"
- or download/compile nexuschat-rpc-server for your platform and
 - either put it into your PATH (for example with "${cargoInstallCommand}")
 - or set the "${ENV_VAR_NAME}" env var to the path to nexuschat-rpc-server"`;
}

export function NPM_NOT_FOUND_UNSUPPORTED_PLATFORM_ERROR() {
  return `nexuschat-rpc-server not found:

Unfortunately no prebuild is available for your system, so you need to provide nexuschat-rpc-server yourself.

- Download or Compile nexuschat-rpc-server for your platform and
 - either put it into your PATH (for example with "${cargoInstallCommand}")
 - or set the "${ENV_VAR_NAME}" env var to the path to nexuschat-rpc-server"`;
}

export function ENV_VAR_LOCATION_NOT_FOUND(error) {
  return `nexuschat-rpc-server not found in ${ENV_VAR_NAME}:

    Error: ${error}

    Content of ${ENV_VAR_NAME}: "${process.env[ENV_VAR_NAME]}"`;
}

export function FAILED_TO_START_SERVER_EXECUTABLE(pathToServerBinary, error) {
  return `Failed to start server executable at '${pathToServerBinary}',

  Error: ${error}

Make sure the nexuschat-rpc-server binary exists at this location 
and you can start it with \`${pathToServerBinary} --version\``;
}
