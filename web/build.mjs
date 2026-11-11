import {cp,mkdir,readdir,writeFile,rm} from "node:fs/promises";
import {join,dirname} from "node:path";
import {fileURLToPath} from "node:url";
const root=dirname(fileURLToPath(import.meta.url)),output=join(root,"dist");
await rm(output,{recursive:true,force:true});
await mkdir(output,{recursive:true});
for(const entry of await readdir(root,{withFileTypes:true})){
 if(entry.name==="dist"||entry.name==="build.mjs"||entry.name==="package.json")continue;
 await cp(join(root,entry.name),join(output,entry.name),{recursive:true});
}
const supabaseUrl=process.env.SUPABASE_URL||"";
const supabaseKey=process.env.SUPABASE_PUBLISHABLE_KEY||"";
if(supabaseUrl && !/^https:\/\/[a-z0-9-]+\.supabase\.co$/.test(supabaseUrl))
 throw Error("SUPABASE_URL must be an https Supabase project URL");
if(supabaseKey && !supabaseKey.startsWith("sb_publishable_"))
 throw Error("Only a public Supabase publishable key is permitted in a web build");
await writeFile(join(output,"config.js"),
 "window.GREEN_ION_CONFIG = Object.freeze("+JSON.stringify({supabaseUrl,supabaseKey})+");\n");
console.log("GREEN ION V14 offline web build created: "+output);
