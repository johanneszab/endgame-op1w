//Decompile every function that calls the given addresses (and the targets themselves).
//Args: <outFile> <comma-separated hex addresses>
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class Callers extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        File out = new File(a[0]);
        // every remaining arg is an address; also accept comma-separated lists
        List<String> tmp = new ArrayList<String>();
        for (int i = 1; i < a.length; i++) {
            for (String p : a[i].split(",")) {
                if (p.trim().length() > 0) { tmp.add(p.trim()); }
            }
        }
        String[] addrs = tmp.toArray(new String[0]);

        Set<Function> want = new LinkedHashSet<Function>();
        StringBuilder index = new StringBuilder();

        for (String s : addrs) {
            Address target = toAddr(s.trim());
            Function tf = getFunctionContaining(target);
            if (tf != null) {
                want.add(tf);
            }
            index.append("\n--- callers of ").append(s.trim())
                 .append(tf == null ? "" : " (" + tf.getName() + ")").append(" ---\n");

            ReferenceIterator ri =
                currentProgram.getReferenceManager().getReferencesTo(target);
            int n = 0;
            while (ri.hasNext()) {
                Reference r = ri.next();
                Function f = getFunctionContaining(r.getFromAddress());
                if (f != null && !f.isThunk()) {
                    if (want.add(f)) { n++; }
                    index.append(String.format("    %-28s @ %s   (call site %s)%n",
                                 f.getName(), f.getEntryPoint(), r.getFromAddress()));
                }
            }
            index.append("    total new: ").append(n).append('\n');
        }

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        di.setSimplificationStyle("decompile");

        PrintWriter w = new PrintWriter(out, "UTF-8");
        w.println("/* index");
        w.println(index.toString());
        w.println("*/");

        int ok = 0;
        for (Function f : want) {
            w.println();
            w.println("/* ======================================================");
            w.println(" * " + f.getName() + " @ " + f.getEntryPoint());
            w.println(" * ====================================================== */");
            DecompileResults res = di.decompileFunction(f, 120, monitor);
            if (res != null && res.decompileCompleted()) {
                w.println(res.getDecompiledFunction().getC());
                ok++;
            } else {
                w.println("/* decompilation failed */");
            }
        }
        di.dispose();
        w.flush();
        w.close();
        println("Callers: decompiled " + ok + "/" + want.size() + " -> " + out);
    }
}
