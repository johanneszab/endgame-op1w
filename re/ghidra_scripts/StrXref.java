//Find a literal string in memory (ASCII and UTF-16LE), list references to it, and decompile the referencing functions.
//Args: <outFile> <string> [<string> ...]
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class StrXref extends GhidraScript {

    private List<Address> findAll(byte[] pat) {
        List<Address> hits = new ArrayList<Address>();
        Memory mem = currentProgram.getMemory();
        Address a = currentProgram.getMinAddress();
        while (a != null) {
            Address f = mem.findBytes(a, pat, null, true, monitor);
            if (f == null) break;
            hits.add(f);
            try { a = f.add(1); } catch (Exception e) { break; }
        }
        return hits;
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");

        Set<Function> want = new LinkedHashSet<Function>();

        for (int i = 1; i < a.length; i++) {
            String s = a[i];
            w.println("=== string \"" + s + "\" ===");

            byte[] ascii = s.getBytes("US-ASCII");
            byte[] wide = new byte[ascii.length * 2];
            for (int k = 0; k < ascii.length; k++) { wide[k * 2] = ascii[k]; }

            List<Address> hits = new ArrayList<Address>();
            for (Address h : findAll(ascii)) { hits.add(h); w.println("  ascii @ " + h); }
            for (Address h : findAll(wide))  { hits.add(h); w.println("  utf16 @ " + h); }

            for (Address h : hits) {
                ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(h);
                int n = 0;
                while (ri.hasNext()) {
                    Reference r = ri.next();
                    Function f = getFunctionContaining(r.getFromAddress());
                    w.println("    ref from " + r.getFromAddress()
                              + (f == null ? "  (no function)" : "  in " + f.getName()
                                 + " @ " + f.getEntryPoint()));
                    if (f != null && !f.isThunk()) { want.add(f); }
                    n++;
                }
                if (n == 0) { w.println("    (no references to " + h + ")"); }
            }
        }

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        di.setSimplificationStyle("decompile");

        for (Function f : want) {
            w.println();
            w.println("/* ======================================================");
            w.println(" * " + f.getName() + " @ " + f.getEntryPoint());
            w.println(" * ====================================================== */");
            DecompileResults res = di.decompileFunction(f, 180, monitor);
            if (res != null && res.decompileCompleted()) {
                w.println(res.getDecompiledFunction().getC());
            } else {
                w.println("/* decompilation failed */");
            }
        }
        di.dispose();
        w.flush();
        w.close();
        println("StrXref: " + want.size() + " functions -> " + a[0]);
    }
}
