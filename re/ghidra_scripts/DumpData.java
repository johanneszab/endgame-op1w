//Dump raw bytes at addresses. Args: <addr:len> ...
//@category EGG
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;

public class DumpData extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (String spec : getScriptArgs()) {
            String[] p = spec.split(":");
            Address a = toAddr(p[0]);
            int len = Integer.parseInt(p[1]);
            byte[] b = new byte[len];
            StringBuilder sb = new StringBuilder();
            try {
                currentProgram.getMemory().getBytes(a, b);
                for (int i = 0; i < len; i++) {
                    sb.append(String.format("%02X ", b[i]));
                }
                // also render as little-endian 16-bit words
                sb.append("  | u16: ");
                for (int i = 0; i + 1 < len; i += 2) {
                    sb.append(String.format("0x%04X ", (b[i] & 0xFF) | ((b[i + 1] & 0xFF) << 8)));
                }
            } catch (Exception e) {
                sb.append("<unreadable: ").append(e.getMessage()).append(">");
            }
            println("DUMP " + p[0] + " : " + sb);
        }
    }
}
