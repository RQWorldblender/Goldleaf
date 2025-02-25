
/*

    Goldleaf - Multipurpose homebrew tool for Nintendo Switch
    Copyright (C) 2018-2020  XorTroll

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <ui/ui_PartitionBrowserLayout.hpp>
#include <ui/ui_MainApplication.hpp>
#include <hos/hos_Payload.hpp>
#include <amssu/amssu_Service.hpp>

extern ui::MainApplication::Ref global_app;
extern cfg::Settings global_settings;

namespace ui
{
    std::vector<u32> g_entry_idx_stack;

    PartitionBrowserLayout::PartitionBrowserLayout() : pu::ui::Layout()
    {
        this->gexp = fs::GetSdCardExplorer();
        this->browseMenu = pu::ui::elm::Menu::New(0, 160, 1280, global_settings.custom_scheme.Base, global_settings.menu_item_size, (560 / global_settings.menu_item_size));
        this->browseMenu->SetOnFocusColor(global_settings.custom_scheme.BaseFocus);
        global_settings.ApplyScrollBarColor(this->browseMenu);
        this->dirEmptyText = pu::ui::elm::TextBlock::New(30, 630, cfg::strings::Main.GetString(49));
        this->dirEmptyText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->dirEmptyText->SetVerticalAlign(pu::ui::elm::VerticalAlign::Center);
        this->dirEmptyText->SetColor(global_settings.custom_scheme.Text);
        this->Add(this->browseMenu);
        this->Add(this->dirEmptyText);
    }

    void PartitionBrowserLayout::ChangePartitionExplorer(fs::Explorer *exp, bool Update)
    {
        this->gexp = exp;
        if(Update) this->UpdateElements();
    }

    void PartitionBrowserLayout::ChangePartitionSdCard(bool Update)
    {
        this->ChangePartitionExplorer(fs::GetSdCardExplorer(), Update);
    }

    void PartitionBrowserLayout::ChangePartitionNAND(fs::Partition Partition, bool Update)
    {
        fs::Explorer *exp = nullptr;
        switch(Partition)
        {
            case fs::Partition::PRODINFOF:
                exp = fs::GetPRODINFOFExplorer();
                break;
            case fs::Partition::NANDSafe:
                exp = fs::GetNANDSafeExplorer();
                break;
            case fs::Partition::NANDUser:
                exp = fs::GetNANDUserExplorer();
                break;
            case fs::Partition::NANDSystem:
                exp = fs::GetNANDSystemExplorer();
                break;
            default:
                return;
        }
        this->ChangePartitionExplorer(exp, Update);
    }
    
    void PartitionBrowserLayout::ChangePartitionPCDrive(String Mount, bool Update)
    {
        this->ChangePartitionExplorer(fs::GetRemotePCExplorer(Mount), Update);
    }

    void PartitionBrowserLayout::ChangePartitionDrive(UsbHsFsDevice &drv, bool Update)
    {
        this->ChangePartitionExplorer(fs::GetDriveExplorer(drv), Update);
    }

    void PartitionBrowserLayout::UpdateElements(int Idx)
    {
        this->elems = this->gexp->GetContents();
        this->browseMenu->ClearItems();
        global_app->LoadMenuHead(this->gexp->GetPresentableCwd());
        if(this->elems.empty())
        {
            this->browseMenu->SetVisible(false);
            this->dirEmptyText->SetVisible(true);
        }
        else
        {
            this->browseMenu->SetVisible(true);
            this->dirEmptyText->SetVisible(false);
            for(auto &itm: this->elems)
            {
                auto mitm = pu::ui::elm::MenuItem::New(itm);
                mitm->SetColor(global_settings.custom_scheme.Text);
                if(this->gexp->IsDirectory(itm)) mitm->SetIcon(global_settings.PathForResource("/FileSystem/Directory.png"));
                else
                {
                    auto ext = LowerCaseString(fs::GetExtension(itm));
                    if(ext == "nsp" || ext == "nsz") mitm->SetIcon(global_settings.PathForResource("/FileSystem/NSP.png"));
                    else if(ext == "nro") mitm->SetIcon(global_settings.PathForResource("/FileSystem/NRO.png"));
                    else if(ext == "tik") mitm->SetIcon(global_settings.PathForResource("/FileSystem/TIK.png"));
                    else if(ext == "cert") mitm->SetIcon(global_settings.PathForResource("/FileSystem/CERT.png"));
                    else if(ext == "nxtheme") mitm->SetIcon(global_settings.PathForResource("/FileSystem/NXTheme.png"));
                    else if(ext == "nca") mitm->SetIcon(global_settings.PathForResource("/FileSystem/NCA.png"));
                    else if(ext == "nacp") mitm->SetIcon(global_settings.PathForResource("/FileSystem/NACP.png"));
                    else if((ext == "jpg") || (ext == "jpeg")) mitm->SetIcon(global_settings.PathForResource("/FileSystem/JPEG.png"));
                    else mitm->SetIcon(global_settings.PathForResource("/FileSystem/File.png"));
                }
                mitm->AddOnClick(std::bind(&PartitionBrowserLayout::fsItems_Click, this, itm));
                mitm->AddOnClick(std::bind(&PartitionBrowserLayout::fsItems_Click_Y, this, itm), KEY_Y);
                this->browseMenu->AddItem(mitm);
            }
            u32 tmpidx = 0;
            if(Idx < 0)
            {
                if(!g_entry_idx_stack.empty())
                {
                    tmpidx = g_entry_idx_stack.back();
                    g_entry_idx_stack.pop_back();
                }
            }
            else
            {
                tmpidx = static_cast<u32>(Idx);
                if(tmpidx >= this->elems.size()) tmpidx = 0;
            }
            this->browseMenu->SetSelectedIndex(tmpidx);
        }
    }

    void PartitionBrowserLayout::HandleFileDirectly(String Path)
    {
        auto dir = fs::GetBaseDirectory(Path);
        auto fname = fs::GetFileName(Path);
        this->ChangePartitionPCDrive(dir);

        auto items = this->browseMenu->GetItems();
        auto it = std::find_if(items.begin(), items.end(), [&](pu::ui::elm::MenuItem::Ref &item) -> bool
        {
            return (item->GetName() == fname);
        });
        if(it == items.end()) return;

        u32 idx = std::distance(items.begin(), it);
        this->browseMenu->SetSelectedIndex(idx);
        fsItems_Click(fname);
    }

    bool PartitionBrowserLayout::GoBack()
    {
        return this->gexp->NavigateBack();
    }

    bool PartitionBrowserLayout::WarnWriteAccess()
    {
        if(!this->gexp->ShouldWarnOnWriteAccess()) return true;
        auto sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(50), cfg::strings::Main.GetString(51), { cfg::strings::Main.GetString(111), cfg::strings::Main.GetString(18) }, true);
        return sopt == 0;
    }

    void PartitionBrowserLayout::fsItems_Click(String item)
    {
        auto fullitm = this->gexp->FullPathFor(item);
        auto pfullitm = this->gexp->FullPresentablePathFor(item);
        if(this->gexp->NavigateForward(fullitm))
        {
            g_entry_idx_stack.push_back(this->browseMenu->GetSelectedIndex());
            this->UpdateElements();
        }
        else
        {
            auto ext = LowerCaseString(fs::GetExtension(item));
            auto msg = cfg::strings::Main.GetString(52) + " ";
            if(ext == "nsp" || ext == "nsz") msg += cfg::strings::Main.GetString(53);
            else if(ext == "nro") msg += cfg::strings::Main.GetString(54);
            else if(ext == "tik") msg += cfg::strings::Main.GetString(55);
            else if(ext == "nxtheme") msg += cfg::strings::Main.GetString(56);
            else if(ext == "nca") msg += cfg::strings::Main.GetString(57);
            else if(ext == "nacp") msg += cfg::strings::Main.GetString(58);
            else if((ext == "jpg") || (ext == "jpeg")) msg += cfg::strings::Main.GetString(59);
            else msg += cfg::strings::Main.GetString(270);
            msg += "\n\n" + cfg::strings::Main.GetString(64) + " " + fs::FormatSize(this->gexp->GetFileSize(fullitm));
            const auto is_bin = this->gexp->IsFileBinary(fullitm);
            std::vector<String> vopts;
            u32 copt = 5;
            if(ext == "nsp" || ext == "nsz")
            {
                vopts.push_back(cfg::strings::Main.GetString(65));
                copt++;
            }
            else if(ext == "nro")
            {
                vopts.push_back(cfg::strings::Main.GetString(66));
                copt++;
            }
            else if(ext == "tik")
            {
                vopts.push_back(cfg::strings::Main.GetString(67));
                copt++;
            }
            else if(ext == "nxtheme")
            {
                vopts.push_back(cfg::strings::Main.GetString(65));
                copt++;
            }
            else if(ext == "nacp")
            {
                vopts.push_back(cfg::strings::Main.GetString(69));
                copt++;
            }
            else if((ext == "jpg") || (ext == "jpeg"))
            {
                vopts.push_back(cfg::strings::Main.GetString(70));
                copt++;
            }
            else if(ext == "bin")
            {
                vopts.push_back(cfg::strings::Main.GetString(66));
                copt++;
            }
            else if(!is_bin)
            {
                vopts.push_back(cfg::strings::Main.GetString(71));
                copt++;
            }
            vopts.push_back(cfg::strings::Main.GetString(72));
            vopts.push_back(cfg::strings::Main.GetString(73));
            vopts.push_back(cfg::strings::Main.GetString(74));
            vopts.push_back(cfg::strings::Main.GetString(75));
            vopts.push_back(cfg::strings::Main.GetString(18));
            auto sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(76), msg, vopts, true);
            if(sopt < 0) return;
            int osopt = sopt;
            if(ext == "nsp" || ext == "nsz")
            {
                switch(sopt)
                {
                    case 0:
                        sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(77), cfg::strings::Main.GetString(78), { cfg::strings::Main.GetString(19), cfg::strings::Main.GetString(79), cfg::strings::Main.GetString(18) }, true);
                        if(sopt < 0) return;
                        Storage dst = Storage::SdCard;
                        if(sopt == 0) dst = Storage::SdCard;
                        else if(sopt == 1) dst = Storage::NANDUser;
                        u64 fsize = this->gexp->GetFileSize(fullitm);
                        u64 rsize = fs::GetFreeSpaceForPartition(static_cast<fs::Partition>(dst));
                        if(rsize < fsize)
                        {
                            HandleResult(err::result::ResultNotEnoughSize, cfg::strings::Main.GetString(251));
                            return;
                        }
                        global_app->LoadMenuHead(cfg::strings::Main.GetString(145) + " " + pfullitm);
                        global_app->LoadLayout(global_app->GetInstallLayout());
                        global_app->GetInstallLayout()->StartInstall(fullitm, this->gexp, dst);
                        global_app->LoadLayout(global_app->GetBrowserLayout());
                        global_app->LoadMenuHead(this->gexp->GetPresentableCwd());
                        break;
                }
            }
            else if(ext == "nro")
            {
                switch(sopt)
                {
                    case 0:
                        if(GetExecutableMode() == ExecutableMode::NRO)
                        {
                            sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(98), cfg::strings::Main.GetString(99), { cfg::strings::Main.GetString(66), cfg::strings::Main.GetString(18) }, true);
                            if(sopt < 0) return;
                            envSetNextLoad(fullitm.AsUTF8().c_str(), fullitm.AsUTF8().c_str());
                            global_app->CloseWithFadeOut();
                            return;
                        }
                        else
                        {
                            global_app->CreateShowDialog(cfg::strings::Main.GetString(98), cfg::strings::Main.GetString(100), { cfg::strings::Main.GetString(234) }, false);
                            return;
                        }
                        break;
                }
            }
            else if(ext == "tik")
            {
                switch(sopt)
                {
                    case 0:
                        sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(101), cfg::strings::Main.GetString(102), { cfg::strings::Main.GetString(234), cfg::strings::Main.GetString(18) }, true);
                        if(sopt == 0)
                        {
                            auto btik = this->gexp->ReadFile(fullitm);
                            Result rc = es::ImportTicket(btik.data(), btik.size(), es::CommonCertificateData, es::CommonCertificateSize);
                            if(R_FAILED(rc)) HandleResult(rc, cfg::strings::Main.GetString(103));
                        }
                        break;
                }
            }
            else if(ext == "nxtheme")
            {
                // TODO: shall we continue supporting this?
                // This implementation is really shitty, and only works for SD files...
                switch(sopt)
                {
                    case 0:
                        std::string nxthemes_nro = "switch/nxthemes_installer/nxthemesinstaller.nro";
                        auto nxthemes_nro_path = "sdmc:/" + nxthemes_nro;
                        auto sd_exp = fs::GetSdCardExplorer();
                        if(!sd_exp->IsFile(nxthemes_nro))
                        {
                            global_app->CreateShowDialog(cfg::strings::Main.GetString(104), cfg::strings::Main.GetString(105), { cfg::strings::Main.GetString(234) }, false);
                            return;
                        }
                        std::string arg = nxthemes_nro_path + " installtheme=" + fullitm.AsUTF8();
                        size_t index = 0;
                        while(true)
                        {
                            index = arg.find(" ", index);
                            if(index == std::string::npos) break;
                            arg.replace(index, 1, "(_)");
                        }
                        envSetNextLoad(nxthemes_nro_path.c_str(), arg.c_str());
                        global_app->CloseWithFadeOut();
                        return;
                        break;
                }
            }
            else if(ext == "nacp") 
            {
                switch(sopt)
                {
                    case 0:
                        NacpStruct nacp = {};
                        auto fsize = this->gexp->GetFileSize(fullitm);
                        if(fsize < sizeof(NacpStruct))
                        {
                            global_app->ShowNotification(cfg::strings::Main.GetString(341));
                            return;
                        }
                        this->gexp->StartFile(fullitm, fs::FileMode::Read);
                        this->gexp->ReadFileBlock(fullitm, 0, sizeof(nacp), &nacp);
                        this->gexp->EndFile(fs::FileMode::Read);
                        NacpStruct *snacp = &nacp;
                        u8 *rnacp = (u8*)snacp;
                        NacpLanguageEntry *lent = nullptr;
                        nacpGetLanguageEntry(snacp, &lent);
                        String name = cfg::strings::Main.GetString(106);
                        String author = cfg::strings::Main.GetString(107);
                        String version = String(snacp->display_version);
                        if(lent != nullptr)
                        {
                            name = String(lent->name);
                            author = String(lent->author);
                        }
                        String msg = cfg::strings::Main.GetString(108) + "\n\n";
                        msg += cfg::strings::Main.GetString(91) + " " + name;
                        msg += "\n" + cfg::strings::Main.GetString(92) + " " + author;
                        msg += "\n" + cfg::strings::Main.GetString(109) + " " + version;
                        msg += "\n" + cfg::strings::Main.GetString(110) + " ";
                        u8 uacc = rnacp[0x3025];
                        if(uacc == 0) msg += cfg::strings::Main.GetString(112);
                        else if(uacc == 1) msg += cfg::strings::Main.GetString(111);
                        else if(uacc == 2) msg += cfg::strings::Main.GetString(113);
                        else msg += cfg::strings::Main.GetString(114);
                        u8 scrc = rnacp[0x3034];
                        msg += "\n" + cfg::strings::Main.GetString(115) + " ";
                        if(scrc == 0) msg += cfg::strings::Main.GetString(111);
                        else if(scrc == 1) msg += cfg::strings::Main.GetString(112);
                        else msg += cfg::strings::Main.GetString(114);
                        u8 vidc = rnacp[0x3035];
                        msg += "\n" + cfg::strings::Main.GetString(116) + " ";
                        if(vidc == 0) msg += cfg::strings::Main.GetString(112);
                        else if(vidc == 1) msg += cfg::strings::Main.GetString(117);
                        else if(vidc == 2) msg += cfg::strings::Main.GetString(111);
                        else msg += cfg::strings::Main.GetString(114);
                        u8 logom = rnacp[0x30f0];
                        msg += "\n" + cfg::strings::Main.GetString(118) + " ";
                        if(logom == 0) msg += cfg::strings::Main.GetString(119);
                        else if(logom == 2) msg += cfg::strings::Main.GetString(120);
                        else msg += cfg::strings::Main.GetString(114);
                        global_app->CreateShowDialog(cfg::strings::Main.GetString(58), msg, { cfg::strings::Main.GetString(234) }, false);
                        break;
                }
            }
            else if((ext == "jpg") || (ext == "jpeg"))
            {
                switch(sopt)
                {
                    case 0:
                        if(!acc::HasUser()) return;
                        sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(121), cfg::strings::Main.GetString(122), { cfg::strings::Main.GetString(111), cfg::strings::Main.GetString(18) }, true);
                        if(sopt < 0) return;

                        size_t fsize = this->gexp->GetFileSize(fullitm);
                        u8 *iconbuf = new u8[fsize]();
                        this->gexp->StartFile(fullitm, fs::FileMode::Read);
                        this->gexp->ReadFileBlock(fullitm, 0, fsize, iconbuf);
                        this->gexp->EndFile(fs::FileMode::Read);

                        auto rc = acc::EditUserIcon(iconbuf, fsize);
                        if(R_SUCCEEDED(rc)) global_app->ShowNotification(cfg::strings::Main.GetString(123));
                        else HandleResult(rc, cfg::strings::Main.GetString(124));
                        delete[] iconbuf;
                        break;
                }
            }
            else if(ext == "bin") 
            {
                switch(sopt)
                {
                    case 0:
                        sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(125), cfg::strings::Main.GetString(126), { cfg::strings::Main.GetString(111), cfg::strings::Main.GetString(18) }, true);
                        if(sopt < 0) return;
                        auto res = hos::RebootWithPayload(fullitm);
                        if(!res) global_app->ShowNotification("Something failed...");
                        break;
                }
            }
            else if(!is_bin)
            {
                switch(sopt)
                {
                    case 0:
                        global_app->LoadLayout(global_app->GetFileContentLayout());
                        global_app->GetFileContentLayout()->LoadFile(pfullitm, fullitm, this->gexp, false);
                        break;
                }
            }
            int viewopt = copt - 5;
            int copyopt = copt - 4;
            int delopt = copt - 3;
            int renopt = copt - 2;
            if((osopt == viewopt) && (this->gexp->GetFileSize(fullitm) > 0))
            {
                global_app->LoadLayout(global_app->GetFileContentLayout());
                global_app->GetFileContentLayout()->LoadFile(pfullitm, fullitm, this->gexp, true);
            }
            else if(osopt == copyopt) UpdateClipboard(fullitm);
            else if(osopt == delopt)
            {
                if(this->WarnWriteAccess())
                {
                    sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(127), cfg::strings::Main.GetString(128), { cfg::strings::Main.GetString(111), cfg::strings::Main.GetString(18) }, true);
                    if(sopt < 0) return;
                    this->gexp->DeleteFile(fullitm);
                    global_app->ShowNotification(cfg::strings::Main.GetString(129));
                    // else HandleResult(rc, cfg::strings::Main.GetString(253));
                    u32 tmpidx = this->browseMenu->GetSelectedIndex();
                    if(tmpidx > 0) tmpidx--;
                    this->UpdateElements(tmpidx);
                }
            }
            else if(osopt == renopt)
            {
                String new_name = AskForText(cfg::strings::Main.GetString(130), item);
                if(!new_name.empty())
                {
                    if(new_name == item) return;
                    auto new_path = this->gexp->FullPathFor(new_name);
                    if(this->gexp->IsFile(new_path) || this->gexp->IsDirectory(new_path)) HandleResult(err::result::ResultEntryAlreadyPresent, cfg::strings::Main.GetString(254));
                    else if(this->WarnWriteAccess())
                    {
                        this->gexp->RenameFile(fullitm, new_path);
                        // if(rc) HandleResult(err::result::MakeErrnoResult(), cfg::strings::Main.GetString(254));
                        global_app->ShowNotification(cfg::strings::Main.GetString(133));
                        this->UpdateElements(this->browseMenu->GetSelectedIndex());
                    }
                }
            }
        }
    }

    void PartitionBrowserLayout::fsItems_Click_Y(String item)
    {
        auto rc = amssu::Initialize();
        auto has_amssu = R_SUCCEEDED(rc);
        
        OnScopeExit ex([&]()
        {
            if(has_amssu) amssu::Exit();
        });

        auto fullitm = this->gexp->FullPathFor(item);
        auto pfullitm = this->gexp->FullPresentablePathFor(item);

        auto ipc_fullitm = this->gexp->RemoveMountName(fullitm);
        amssu::UpdateInformation update_info = {};
        rc = amssu::GetUpdateInformation(ipc_fullitm.AsUTF8().c_str(), &update_info);
        auto is_valid_update = R_SUCCEEDED(rc);

        if(this->gexp->IsDirectory(fullitm))
        {
            auto files = this->gexp->GetFiles(fullitm);
            std::vector<String> nsps;
            for(auto &file: files)
            {
                auto path = fullitm + "/" + file;
                auto ext = LowerCaseString(fs::GetExtension(path));
                if(ext == "nsp" || ext == "nsz") nsps.push_back(file);
            }
            std::vector<String> extraopts = { cfg::strings::Main.GetString(281) };
            if(!nsps.empty()) extraopts.push_back(cfg::strings::Main.GetString(282));
            extraopts.push_back(cfg::strings::Main.GetString(18));
            String msg = cfg::strings::Main.GetString(134);
            msg += "\n\n" + cfg::strings::Main.GetString(237) + " " + fs::FormatSize(this->gexp->GetDirectorySize(fullitm));
            int sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(135), msg, { cfg::strings::Main.GetString(415), cfg::strings::Main.GetString(73), cfg::strings::Main.GetString(74), cfg::strings::Main.GetString(75), cfg::strings::Main.GetString(280), cfg::strings::Main.GetString(18) }, true);
            if(sopt < 0) return;
            switch(sopt)
            {
                case 0:
                    if(is_valid_update)
                    {
                        String update_msg = "";
                        String version_str = std::to_string((update_info.version >> 26) & 0x1F) + "." + std::to_string((update_info.version >> 20) & 0x1F) + "." + std::to_string((update_info.version >> 16) & 0xF);
                        update_msg += " - " + cfg::strings::Main.GetString(417) + " " + version_str + "\n\n";
                        update_msg += " - " + cfg::strings::Main.GetString(418) + " " + (update_info.exfat_supported ? cfg::strings::Main.GetString(111) : cfg::strings::Main.GetString(112));
                        
                        sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(416), update_msg, { cfg::strings::Main.GetString(419), cfg::strings::Main.GetString(18) }, true);
                        if(sopt == 0)
                        {
                            amssu::UpdateValidationInfo update_validation_info = {};
                            auto rc = amssu::ValidateUpdate(ipc_fullitm.AsUTF8().c_str(), &update_validation_info);
                            if(R_SUCCEEDED(rc) && R_SUCCEEDED(update_validation_info.result) && R_SUCCEEDED(update_validation_info.exfat_result))
                            {
                                auto sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(420), cfg::strings::Main.GetString(421), { cfg::strings::Main.GetString(423), cfg::strings::Main.GetString(18) }, true);
                                if(sopt == 0)
                                {
                                    sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(424), cfg::strings::Main.GetString(425), { cfg::strings::Main.GetString(111), cfg::strings::Main.GetString(112), cfg::strings::Main.GetString(18) }, true);
                                    if((sopt == 0) || (sopt == 1))
                                    {
                                        auto with_exfat = sopt == 0;
                                        SetSysFirmwareVersion fwver = {};
                                        setsysGetFirmwareVersion(&fwver);
                                        global_app->LoadMenuData(cfg::strings::Main.GetString(424), "Update", String(fwver.display_version) + " → " + version_str + "...");
                                        global_app->LoadLayout(global_app->GetUpdateInstallLayout());
                                        global_app->GetUpdateInstallLayout()->InstallUpdate(ipc_fullitm, with_exfat);
                                    }
                                }
                            }
                            else
                            {
                                auto failed_rc = 0;
                                if(R_FAILED(rc)) failed_rc = rc;
                                if(R_FAILED(update_validation_info.result)) failed_rc = update_validation_info.result;
                                if(R_FAILED(update_validation_info.exfat_result)) failed_rc = update_validation_info.exfat_result;
                                HandleResult(failed_rc, cfg::strings::Main.GetString(422));
                            }
                        }
                    }
                    else global_app->ShowNotification(cfg::strings::Main.GetString(433));
                    break;
                case 1:
                    UpdateClipboard(fullitm);
                    break;
                case 2:
                    if(this->WarnWriteAccess())
                    {
                        sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(325), cfg::strings::Main.GetString(326), {cfg::strings::Main.GetString(111), cfg::strings::Main.GetString(18)}, true);
                        if(sopt < 0) return;
                        this->gexp->DeleteDirectory(fullitm);
                        global_app->ShowNotification(cfg::strings::Main.GetString(327));
                        this->UpdateElements();
                    }
                    break;
                case 3:
                    {
                        String kbdt = AskForText(cfg::strings::Main.GetString(238), item);
                        if(kbdt != "")
                        {
                            if(kbdt == item) return;
                            String newren = this->gexp->FullPathFor(kbdt);
                            if(this->gexp->IsFile(newren) || this->gexp->IsDirectory(newren)) HandleResult(err::result::ResultEntryAlreadyPresent, cfg::strings::Main.GetString(254));
                            else if(this->WarnWriteAccess())
                            {
                                int rc = 0;
                                this->gexp->RenameDirectory(fullitm, newren);
                                if(rc) HandleResult(rc, cfg::strings::Main.GetString(254));
                                else global_app->ShowNotification(cfg::strings::Main.GetString(139));
                                this->UpdateElements();
                            }
                        }
                    }
                    break;
                case 4:
                    int sopt2 = global_app->CreateShowDialog(cfg::strings::Main.GetString(280), cfg::strings::Main.GetString(134), extraopts, true);
                    switch(sopt2)
                    {
                        case 0:
                            this->gexp->SetArchiveBit(fullitm);
                            this->UpdateElements(this->browseMenu->GetSelectedIndex());
                            global_app->ShowNotification(cfg::strings::Main.GetString(303));
                            break;
                        case 1:
                            sopt = global_app->CreateShowDialog(cfg::strings::Main.GetString(77), cfg::strings::Main.GetString(78), { cfg::strings::Main.GetString(19), cfg::strings::Main.GetString(79), cfg::strings::Main.GetString(18) }, true);
                            if(sopt < 0) return;
                            Storage dst = Storage::SdCard;
                            if(sopt == 0) dst = Storage::SdCard;
                            else if(sopt == 1) dst = Storage::NANDUser;
                            for(auto &nsp_path: nsps)
                            {
                                auto nsp = fullitm + "/" + nsp_path;
                                auto pnsp = pfullitm + "/" + nsp_path;
                                
                                u64 fsize = this->gexp->GetFileSize(nsp);
                                u64 rsize = fs::GetFreeSpaceForPartition(static_cast<fs::Partition>(dst));
                                if(rsize < fsize)
                                {
                                    HandleResult(err::result::ResultNotEnoughSize, cfg::strings::Main.GetString(251));
                                    return;
                                }
                                global_app->LoadMenuHead(cfg::strings::Main.GetString(145) + " " + pnsp);
                                global_app->LoadLayout(global_app->GetInstallLayout());
                                global_app->GetInstallLayout()->StartInstall(nsp, this->gexp, dst, true);
                                global_app->LoadLayout(global_app->GetBrowserLayout());
                            }
                            global_app->LoadMenuHead(this->gexp->GetPresentableCwd());
                            break;
                    }
                    break;
            }
        }
    }

    fs::Explorer *PartitionBrowserLayout::GetExplorer()
    {
        return this->gexp;
    }
}
