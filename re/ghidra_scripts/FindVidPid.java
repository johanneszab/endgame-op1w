//Find every code reference to a vendor ID and report the PIDs used with it.
//Args: <outFile> [vendorIdHex, default 3367]
//@category EGG
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.OperandType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;

public class FindVidPid extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        File out = new File(a[0]);
        long vid = a.length > 1 ? Long.parseLong(a[1], 16) : 0x3367L;

        PrintWriter w = new PrintWriter(new FileWriter(out, true));
        w.println();
        w.println("################################################################");
        w.println("### " + currentProgram.getName());
        w.println("################################################################");

        // Instructions that mention the vendor ID as an immediate.
        Set<Function> hits = new LinkedHashSet<Function>();
        List<String> sites = new ArrayList<String>();

        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            boolean match = false;
            for (int i = 0; i < ins.getNumOperands(); i++) {
                Scalar s = ins.getScalar(i);
                if (s != null && s.getUnsignedValue() == vid) {
                    match = true;
                }
                if ((ins.getOperandType(i) & OperandType.SCALAR) != 0) {
                    Object[] objs = ins.getOpObjects(i);
                    for (Object o : objs) {
                        if (o instanceof Scalar
                                && ((Scalar) o).getUnsignedValue() == vid) {
                            match = true;
                        }
                    }
                }
            }
            if (!match) {
                continue;
            }
            Function f = getFunctionContaining(ins.getAddress());
            sites.add(String.format("    %s   %-30s  in %s",
                      ins.getAddress(), ins.toString(),
                      f == null ? "<none>" : f.getName()));
            if (f != null) {
                hits.add(f);
            }
        }

        w.println();
        w.printf("--- %d instruction(s) referencing VID 0x%04X ---%n", sites.size(), vid);
        for (String s : sites) {
            w.println(s);
        }

        // Decompile the containing functions and keep only the lines that
        // mention the vendor ID, which is where the PID sits next to it.
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        di.setSimplificationStyle("decompile");

        String needle = String.format("0x%x", vid);
        w.println();
        w.println("--- decompiled call sites ---");
        for (Function f : hits) {
            DecompileResults res = di.decompileFunction(f, 120, monitor);
            if (res == null || !res.decompileCompleted()) {
                w.println("    " + f.getName() + " : <decompilation failed>");
                continue;
            }
            String[] lines = res.getDecompiledFunction().getC().split("\n");
            for (int i = 0; i < lines.length; i++) {
                if (lines[i].toLowerCase().contains(needle)) {
                    w.printf("    %-28s %s%n", f.getName(), lines[i].trim());
                }
            }
        }
        di.dispose();

        w.flush();
        w.close();
        println("FindVidPid: " + sites.size() + " site(s) in " + currentProgram.getName());
    }
}
