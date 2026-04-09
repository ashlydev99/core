#!/usr/bin/env node
import { readFileSync, writeFileSync } from "fs";
import { resolve } from "path";
import { fileURLToPath } from "url";
import { dirname } from "path";
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const data = [];
const header = resolve(__dirname, "../../../nexuschat-ffi/nexuschat.h");

console.log("Generating constants...");

const header_data = readFileSync(header, "UTF-8");
const regex = /^#define\s+(\w+)\s+(\w+)/gm;
let match;
while (null != (match = regex.exec(header_data))) {
  const key = match[1];
  const value = parseInt(match[2]);
  if (!isNaN(value)) {
    data.push({ key, value });
  }
}

const constants = data
  .filter(
    ({ key }) => key.toUpperCase()[0] === key[0], // check if define name is uppercase
  )
  .sort((lhs, rhs) => {
    if (lhs.key < rhs.key) return -1;
    else if (lhs.key > rhs.key) return 1;
    return 0;
  })
  .filter(({ key }) => {
    // filter out what we don't need it
    return !(
      key.startsWith("NC_EVENT_") ||
      key.startsWith("NC_IMEX_") ||
      key.startsWith("NC_CHAT_VISIBILITY") ||
      key.startsWith("NC_DOWNLOAD") ||
      key.startsWith("NC_INFO_") ||
      (key.startsWith("NC_MSG") && !key.startsWith("NC_MSG_ID")) ||
      key.startsWith("NC_QR_") ||
      key.startsWith("NC_CERTCK_") ||
      key.startsWith("NC_SOCKET_") ||
      key.startsWith("NC_LP_AUTH_") ||
      key.startsWith("NC_PUSH_") ||
      key.startsWith("NC_TEXT1_") ||
      key.startsWith("NC_CHAT_TYPE")
    );
  })
  .map((row) => {
    return `  export const ${row.key} = ${row.value};`;
  })
  .join("\n");

writeFileSync(
  resolve(__dirname, "../generated/constants.ts"),
  `// Generated!

export namespace C {
${constants}
  /** @deprecated 10-8-2025 compare string directly with \`== "Group"\` */
  export const NC_CHAT_TYPE_GROUP = "Group";
  /** @deprecated 10-8-2025 compare string directly with \`== "InBroadcast"\`*/
  export const NC_CHAT_TYPE_IN_BROANCAST = "InBroadcast";
  /** @deprecated 10-8-2025 compare string directly with \`== "Mailinglist"\` */
  export const NC_CHAT_TYPE_MAILINGLIST = "Mailinglist";
  /** @deprecated 10-8-2025 compare string directly with \`== "OutBroadcast"\` */
  export const NC_CHAT_TYPE_OUT_BROANCAST = "OutBroadcast";
  /** @deprecated 10-8-2025 compare string directly with \`== "Single"\` */
  export const NC_CHAT_TYPE_SINGLE = "Single";
}\n`,
);
