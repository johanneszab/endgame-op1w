//Decompile exactly the functions containing the given addresses.
//Args: <outFile> <hex addr> [<hex addr> ...]
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;

public class Decomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");
        List<String> args = new ArrayList<String>();
        for (int i = 1; i < a.length; i++)
            for (String p : a[i].split(",")) if (p.trim().length() > 0) args.add(p.trim());

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        di.setSimplificationStyle("decompile");
        for (String s : args) {
            Address ad = toAddr(s);
            Function f = getFunctionContaining(ad);
            if (f == null) { w.println("/* no function at " + s + " */"); continue; }
            w.println("/* ============ " + f.getName() + " @ " + f.getEntryPoint()
                      + "  (req " + s + ") ============ */");
            // list callees
            w.print("/* calls:");
            for (Function c : f.getCalledFunctions(monitor)) w.print(" " + c.getName() + "@" + c.getEntryPoint());
            w.println(" */");
            w.print("/* called by:");
            ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint());
            Set<String> seen = new LinkedHashSet<String>();
            while (ri.hasNext()) {
                Function g = getFunctionContaining(ri.next().getFromAddress());
                if (g != null) seen.add(g.getName() + "@" + g.getEntryPoint());
            }
            for (String s2 : seen) w.print(" " + s2);
            w.println(" */");
            DecompileResults res = di.decompileFunction(f, 180, monitor);
            if (res != null && res.decompileCompleted()) w.println(res.getDecompiledFunction().getC());
            else w.println("/* decompilation failed: " + (res == null ? "null" : res.getErrorMessage()) + " */");
            w.println();
        }
        di.dispose();
        w.flush(); w.close();
        println("Decomp done -> " + a[0]);
    }
}
