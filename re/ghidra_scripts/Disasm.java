//Dump raw disassembly (bytes + mnemonics) for a function or address range.
//Args: <outFile> <hexAddr>[:<count>] ...   count = number of instructions (default: whole function)
//@category EGG
import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;

public class Disasm extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");
        for (int i = 1; i < a.length; i++) {
            String spec = a[i];
            int limit = -1;
            if (spec.contains(":")) {
                String[] p = spec.split(":");
                spec = p[0];
                limit = Integer.parseInt(p[1]);
            }
            Address start = toAddr(spec);
            Function f = getFunctionContaining(start);
            Address end = null;
            if (f != null && limit < 0) {
                end = f.getBody().getMaxAddress();
            }
            w.println();
            w.println("==== " + spec + (f == null ? "" : "  (" + f.getName()
                      + " @ " + f.getEntryPoint() + ")") + " ====");
            InstructionIterator it = currentProgram.getListing().getInstructions(start, true);
            int n = 0;
            while (it.hasNext()) {
                Instruction ins = it.next();
                if (end != null && ins.getAddress().compareTo(end) > 0) { break; }
                StringBuilder b = new StringBuilder();
                for (byte by : ins.getBytes()) { b.append(String.format("%02x ", by)); }
                w.println(String.format("%s  %-24s %s", ins.getAddress(), b.toString(), ins.toString()));
                n++;
                if (limit > 0 && n >= limit) { break; }
            }
        }
        w.flush();
        w.close();
        println("Disasm: done -> " + a[0]);
    }
}
