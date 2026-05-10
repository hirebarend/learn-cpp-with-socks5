import axios from "axios";
import { SocksProxyAgent } from "socks-proxy-agent";

await new Promise((resolve) => setTimeout(resolve, 3000));

const uri = "socks5://username:password@127.0.0.1:8080";

const socksProxyAgent = new SocksProxyAgent(uri);

try {
  const res = await axios.get("https://api.ipify.org?format=json", {
    httpAgent: socksProxyAgent,
    httpsAgent: socksProxyAgent,
    proxy: false,
    timeout: 15000,
  });

  console.log(res.data);
} catch {}
