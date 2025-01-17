// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "UI/RAbilitySlot.h"
#include "Components/HorizontalBox.h"
#include "Animation/WidgetAnimation.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/RadiationWidget.h"
#include "UI/QAbilitySlot.h"
#include "UI/RAbilitySlot.h"
#include "UI/IconWidget.h"
#include "Components/CanvasPanel.h"
#include "UserView.generated.h"


class AChel;
/**
 * 
 */

UCLASS()
class MYPROJECT9_API UUserView : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;
	/*void AddDoubleRadiationEffect();
	void DisableDoubleRadiationEffect();
	void AddFreezeEffect();
	void DisableFreezeEffect();*/
	//void Construct() override;

	void AddIconToPanel(int32 IdEffect);
	UFUNCTION(BlueprintCallable)
		void RemoveIconFromPanel(int32 IdEffect);

	void ShowTaskOfGame(int32 AmountPlayersInGame);
public:

	UPROPERTY(EditAnywhere, Category = "Effect Icon Classes")
		TSubclassOf<UIconWidget> RadiationRandomEvent_Class;
	UPROPERTY(EditAnywhere, Category = "Effect Icon Classes")
		TSubclassOf<UIconWidget> RadiationVentil_Class;
	UPROPERTY(EditAnywhere, Category = "Effect Icon Classes")
		TSubclassOf<UIconWidget> Invisible_Class;
	UPROPERTY(EditAnywhere, Category = "Effect Icon Classes")
		TSubclassOf<UIconWidget> DecreaseRadiation_Class;
	UPROPERTY(EditAnywhere, Category = "Effect Icon Classes")
		TSubclassOf<UIconWidget> Immortal_Class;
	UPROPERTY(EditAnywhere, Category = "Effect Icon Classes")
		TSubclassOf<UIconWidget> DoubleDamage_Class;
	UPROPERTY(EditAnywhere, Category = "Effect Icon Classes")
		TSubclassOf<UIconWidget> InvertMovement_Class;



	AChel* Player;
	//---------------------------
	UPROPERTY(meta = (BindWidget))
		UTextBlock* AreaUsedText;

	UPROPERTY(meta = (BindWidget))
		UHorizontalBox* Effects_Bar;

	UPROPERTY(meta = (BindWidget))
		UImage* E_Mark;
	//---------------------------

	UPROPERTY(meta = (BindWidget))
		UTextBlock* WeaponName;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* AmmoSlash;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
		UTextBlock* PropmptTextInterract;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
		UTextBlock* PropmptTextArea;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
		UTextBlock* CountLeftPromt;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* AmmoLabel;

	TArray<UTextBlock*> ArrayCacheKey;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* KeyLeft_Bronze;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* KeyLeft_Silver;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* KeyLeft_Gold;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* AmmoMax;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* Details;
	
	//---------------------------
	UPROPERTY(meta = (BindWidget)) //!!!!!!!!!!!!!!!!!!!!!!!
		UTextBlock* TimeLeft;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* TB_Task4Player;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* TB_Task3Player;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* TB_Task2Player;

	UPROPERTY(meta = (BindWidget))
		UCanvasPanel* CP_HiddenItems;
	//---------------------------
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))  //!!!!!!!!!!!!!!!!!!!!!!!
		UTextBlock* HoldText;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* EscapeText;

	//---------------------------
	UPROPERTY(meta = (BindWidget))
		UProgressBar* RadiationPoints;
	//---------------------------
	UPROPERTY(meta = (BindWidget)) //!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		UProgressBar* PB_Opening;
	//---------------------------
	UPROPERTY(meta = (BindWidget))
		UImage* DarkScreen;
	//---------------------------
	UPROPERTY(meta = (BindWidget))
		UImage* Marker;
	//---------------------------

	TArray<UWidgetSwitcher*> ArraySwitcher;
	UPROPERTY(meta = (BindWidget))
		UWidgetSwitcher* WS_Boltorez;
	
	UPROPERTY(meta = (BindWidget))
		UWidgetSwitcher* WS_KeyShelter;

	UPROPERTY(meta = (BindWidget))
		UWidgetSwitcher* WS_Otvertka;
	//---------------------------

	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* Shading;

	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* CanalizaciaAnim;

	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* ShelterAnim;

	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* VentilaciaAnim;
	
	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* OpenAreaAnim;

	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* HideTextAnimation2;

	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* HideTextAnimation3;

	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* HideTextAnimation4;
	
	UPROPERTY(meta = (BindWidgetAnim))
		UWidgetAnimation* HideAdditiveInformation;

	UPROPERTY(EditAnywhere, Category = "UI HUD")
		TSubclassOf<URadiationWidget> RadiationImage;
	
	UPROPERTY(EditAnywhere, Category = "UI HUD")
		TSubclassOf<UUserWidget> FreezeImage;

	UPROPERTY(meta = (BindWidget))
		URAbilitySlot* CurRSlot;

	UPROPERTY(meta = (BindWidget))
		UQAbilitySlot* CurQSlot;

	void NativeConstruct() override;

	bool IsAwake = true;
};
