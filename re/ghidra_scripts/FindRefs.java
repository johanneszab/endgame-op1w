//List every reference to the given data addresses: containing function, instruction text, access type.
//Args: <outFile> <hexAddr> [<hexAddr> ...]
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class FindRefs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");
        List<String> addrs = new ArrayList<String>();
        for (int i = 1; i < a.length; i++)
            for (String p : a[i].split(","))
                if (p.trim().length() > 0) addrs.add(p.trim());

        for (String s : addrs) {
            Address t = toAddr(s);
            w.println("=== refs to " + s + " ===");
            ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(t);
            Set<String> funcs = new LinkedHashSet<String>();
            while (ri.hasNext()) {
                Reference r = ri.next();
                Address from = r.getFromAddress();
                Function f = getFunctionContaining(from);
                Instruction ins = getInstructionAt(from);
                w.println(String.format("  %s  %-14s @ %s  [%s]  %s",
                        from,
                        f == null ? "(no func)" : f.getName(),
                        f == null ? "-" : f.getEntryPoint().toString(),
                        r.getReferenceType(),
                        ins == null ? "(not an instruction)" : ins.toString()));
                if (f != null) funcs.add(f.getName() + " @ " + f.getEntryPoint());
            }
            w.println("  -- distinct functions: " + funcs);
        }
        w.flush(); w.close();
        println("FindRefs done -> " + a[0]);
    }
}
