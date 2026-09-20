//Locate HID API call sites and decompile their containing functions.
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
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;

public class FindHid extends GhidraScript {

    static final String[] TARGETS = {
        "HidD_SetFeature", "HidD_GetFeature", "HidD_GetAttributes",
        "HidD_GetPreparsedData", "HidP_GetCaps", "HidD_SetNumInputBuffers",
        "HidD_GetProductString", "CreateFileW", "CreateFileA",
        "ReadFile", "WriteFile", "DeviceIoControl"
    };

    @Override
    public void run() throws Exception {
        String outDir = getScriptArgs().length > 0 ? getScriptArgs()[0] : ".";
        PrintWriter xw = new PrintWriter(new File(outDir, "hid_xrefs.txt"), "UTF-8");
        PrintWriter dw = new PrintWriter(new File(outDir, "hid_decomp.c"), "UTF-8");

        // name -> set of functions that call it
        Map<String, Set<Function>> callers = new LinkedHashMap<String, Set<Function>>();
        Set<Function> toDecompile = new LinkedHashSet<Function>();

        for (String t : TARGETS) {
            Set<Function> set = new LinkedHashSet<Function>();
            callers.put(t, set);

            List<Address> entries = new ArrayList<Address>();
            SymbolIterator si = currentProgram.getSymbolTable().getSymbolIterator(t, true);
            while (si.hasNext()) {
                Symbol s = si.next();
                entries.add(s.getAddress());
            }
            for (Function f : getGlobalFunctions(t)) {
                entries.add(f.getEntryPoint());
            }

            for (Address a : entries) {
                ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(a);
                while (ri.hasNext()) {
                    Reference r = ri.next();
                    Function f = getFunctionContaining(r.getFromAddress());
                    if (f != null && !f.isThunk()) {
                        set.add(f);
                        // Only decompile the interesting (HID-specific) callers.
                        if (t.startsWith("Hid")) {
                            toDecompile.add(f);
                        }
                    }
                }
            }
        }

        xw.println("=== HID / file API call sites ===");
        for (Map.Entry<String, Set<Function>> e : callers.entrySet()) {
            xw.println();
            xw.println("--- " + e.getKey() + "  (" + e.getValue().size() + " calling function(s)) ---");
            for (Function f : e.getValue()) {
                xw.println(String.format("    %s  @ %s", f.getName(), f.getEntryPoint()));
            }
        }
        xw.flush();
        xw.close();

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        di.setSimplificationStyle("decompile");

        dw.println("/* Decompiled callers of HidD_*/HidP_* in "
                   + currentProgram.getName() + " */");
        for (Function f : toDecompile) {
            dw.println();
            dw.println("/* ======================================================");
            dw.println(" * " + f.getName() + " @ " + f.getEntryPoint());
            dw.println(" * ====================================================== */");
            DecompileResults res = di.decompileFunction(f, 120, monitor);
            if (res != null && res.decompileCompleted()) {
                dw.println(res.getDecompiledFunction().getC());
            } else {
                dw.println("/* decompilation failed: "
                           + (res == null ? "null" : res.getErrorMessage()) + " */");
            }
        }
        di.dispose();
        dw.flush();
        dw.close();

        println("FindHid: wrote hid_xrefs.txt and hid_decomp.c to " + outDir
                + " (" + toDecompile.size() + " functions decompiled)");
    }
}
