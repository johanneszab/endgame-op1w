//Find functions whose name matches a regex; print address and decompile each.
//Args: <outFile> <regex> [<regex> ...]   (use "LIST:" prefix on a regex to only list, not decompile)
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;
import java.util.regex.Pattern;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class ByName extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        di.setSimplificationStyle("decompile");

        for (int i = 1; i < a.length; i++) {
            String spec = a[i];
            boolean listOnly = false;
            if (spec.startsWith("LIST:")) { listOnly = true; spec = spec.substring(5); }
            Pattern p = Pattern.compile(spec, Pattern.CASE_INSENSITIVE);
            w.println("################ regex " + spec);
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            int n = 0;
            List<Function> hits = new ArrayList<Function>();
            while (it.hasNext()) {
                Function f = it.next();
                if (p.matcher(f.getName()).find()) { hits.add(f); }
            }
            for (Function f : hits) {
                w.println("### " + f.getName() + " @ " + f.getEntryPoint()
                          + (f.isThunk() ? "  [THUNK -> " + f.getThunkedFunction(true).getName()
                             + " @ " + f.getThunkedFunction(true).getEntryPoint() + "]" : ""));
                n++;
            }
            w.println("### total hits: " + n);
            if (listOnly) { continue; }
            for (Function f : hits) {
                Function t = f.isThunk() ? f.getThunkedFunction(true) : f;
                w.println();
                w.println("/* ======== " + t.getName() + " @ " + t.getEntryPoint() + " ======== */");
                DecompileResults r = di.decompileFunction(t, 120, monitor);
                if (r.decompileCompleted() && r.getDecompiledFunction() != null) {
                    w.println(r.getDecompiledFunction().getC());
                } else {
                    w.println("/* decompile failed: " + r.getErrorMessage() + " */");
                }
            }
        }
        w.close();
        println("ByName done -> " + a[0]);
    }
}
