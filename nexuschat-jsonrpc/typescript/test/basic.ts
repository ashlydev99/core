import chai, { assert, expect } from "chai";
import chaiAsPromised from "chai-as-promised";
chai.use(chaiAsPromised);
import { StdioNexusChat as NexusChat } from "../nexuschat.js";

import { RpcServerHandle, startServer } from "./test_base.js";

describe("basic tests", () => {
  let serverHandle: RpcServerHandle;
  let nc: NexusChat;

  before(async () => {
    serverHandle = await startServer();
    nc = new NexusChat(serverHandle.stdin, serverHandle.stdout, true);
    // nc.on("ALL", (event) => {
    //console.log("event", event);
    // });
  });

  after(async () => {
    nc && nc.close();
    await serverHandle.close();
  });

  it("check email address validity", async () => {
    const validAddresses = [
      "email@example.com",
      "36aa165ae3406424e0c61af17700f397cad3fe8ab83d682d0bddf3338a5dd52e@yggmail@yggmail",
    ];
    const invalidAddresses = ["email@", "example.com", "emai221"];

    expect(
      await Promise.all(
        validAddresses.map((email) => nc.rpc.checkEmailValidity(email)),
      ),
    ).to.not.contain(false);

    expect(
      await Promise.all(
        invalidAddresses.map((email) => nc.rpc.checkEmailValidity(email)),
      ),
    ).to.not.contain(true);
  });

  it("system info", async () => {
    const systemInfo = await nc.rpc.getSystemInfo();
    expect(systemInfo).to.contain.keys([
      "arch",
      "num_cpus",
      "nexuschat_core_version",
      "sqlite_version",
    ]);
  });

  describe("account management", () => {
    it("should create account", async () => {
      const res = await nc.rpc.addAccount();
      assert((await nc.rpc.getAllAccountIds()).length === 1);
    });

    it("should remove the account again", async () => {
      await nc.rpc.removeAccount((await nc.rpc.getAllAccountIds())[0]);
      assert((await nc.rpc.getAllAccountIds()).length === 0);
    });

    it("should create multiple accounts", async () => {
      await nc.rpc.addAccount();
      await nc.rpc.addAccount();
      await nc.rpc.addAccount();
      await nc.rpc.addAccount();
      assert((await nc.rpc.getAllAccountIds()).length === 4);
    });
  });

  describe("contact management", function () {
    let accountId: number;
    before(async () => {
      accountId = await nc.rpc.addAccount();
    });
    it("should block and unblock contact", async function () {
      // Cannot send sync messages to self as we do not have a self address.
      await nc.rpc.setConfig(accountId, "sync_msgs", "0");

      const contactId = await nc.rpc.createContact(
        accountId,
        "example@nexus.chat",
        null,
      );
      expect((await nc.rpc.getContact(accountId, contactId)).isBlocked).to.be
        .false;
      await nc.rpc.blockContact(accountId, contactId);
      expect((await nc.rpc.getContact(accountId, contactId)).isBlocked).to.be
        .true;
      expect(await nc.rpc.getBlockedContacts(accountId)).to.have.length(1);
      await nc.rpc.unblockContact(accountId, contactId);
      expect((await nc.rpc.getContact(accountId, contactId)).isBlocked).to.be
        .false;
      expect(await nc.rpc.getBlockedContacts(accountId)).to.have.length(0);
    });
  });

  describe("configuration", function () {
    let accountId: number;
    before(async () => {
      accountId = await nc.rpc.addAccount();
    });

    it("set and retrieve", async function () {
      await nc.rpc.setConfig(accountId, "addr", "valid@email");
      assert((await nc.rpc.getConfig(accountId, "addr")) == "valid@email");
    });
    it("set invalid key should throw", async function () {
      await expect(nc.rpc.setConfig(accountId, "invalid_key", "some value")).to
        .be.eventually.rejected;
    });
    it("get invalid key should throw", async function () {
      await expect(nc.rpc.getConfig(accountId, "invalid_key")).to.be.eventually
        .rejected;
    });
    it("set and retrieve ui.*", async function () {
      await nc.rpc.setConfig(accountId, "ui.chat_bg", "color:red");
      assert((await nc.rpc.getConfig(accountId, "ui.chat_bg")) == "color:red");
    });
    it("set and retrieve (batch)", async function () {
      const config = { addr: "valid@email", mail_pw: "1234" };
      await nc.rpc.batchSetConfig(accountId, config);
      const retrieved = await nc.rpc.batchGetConfig(
        accountId,
        Object.keys(config),
      );
      expect(retrieved).to.deep.equal(config);
    });
    it("set and retrieve ui.* (batch)", async function () {
      const config = {
        "ui.chat_bg": "color:green",
        "ui.enter_key_sends": "true",
      };
      await nc.rpc.batchSetConfig(accountId, config);
      const retrieved = await nc.rpc.batchGetConfig(
        accountId,
        Object.keys(config),
      );
      expect(retrieved).to.deep.equal(config);
    });
    it("set and retrieve mixed(ui and core) (batch)", async function () {
      const config = {
        "ui.chat_bg": "color:yellow",
        "ui.enter_key_sends": "false",
        addr: "valid2@email",
        mail_pw: "123456",
      };
      await nc.rpc.batchSetConfig(accountId, config);
      const retrieved = await nc.rpc.batchGetConfig(
        accountId,
        Object.keys(config),
      );
      expect(retrieved).to.deep.equal(config);
    });
  });
});
