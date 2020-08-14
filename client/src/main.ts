import client from "./client";

async function main() {
  const twoSecondsLater = Date.now() + 2000;
  try {
    await new Promise((resolve, reject) =>
      client.waitForReady(twoSecondsLater, (err: any) =>
        err ? reject(err) : resolve()
      )
    );
  } catch (err) {
    console.error("could not connect to server within timeout");
  }

  await new Promise((resolve, reject) =>
    client.SayA({ name: "hello" }, (err: any) =>
      err ? reject(err) : resolve(err)
    )
  );

  for (let i = 0; i < 5000; ++i) {
    console.log(`try ${i}`);
    await new Promise((resolve, reject) =>
      client.SayB(
        { name: "world", bytes: Buffer.allocUnsafe(100) },
        (err: any) => (err ? reject(err) : resolve(err))
      )
    );
  }
}

main().catch(console.error);
