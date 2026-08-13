import { cp, mkdir, rm } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const output = resolve(root, "dist");

await rm(output, { recursive: true, force: true });
await mkdir(output, { recursive: true });

for (const entry of ["index.html", "resources", "editor_images"]) {
  await cp(resolve(root, entry), resolve(output, entry), { recursive: true });
}

console.log(`Web assets copied to ${output}`);
