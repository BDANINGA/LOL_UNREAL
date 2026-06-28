#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PC_GameStart.generated.h"

class UButton;
class UEditableTextBox;
class UOverlay;
class UTextBlock;
class UUserWidget;

UCLASS()
class LOL_UNREAL_API APC_GameStart : public APlayerController
{
	GENERATED_BODY()

public:
	APC_GameStart();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> GameStartWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> GameStartWidget;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> NicknameInput;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> IpAddressInput;

	UPROPERTY()
	TObjectPtr<UOverlay> IpInputOverlay;

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleIpTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void BindGameStartWidget();
	UEditableTextBox* ReplaceTextBlockWithEditable(UTextBlock* OriginalText, FName EditableName);
	bool SaveNickname();
	void JoinLobby();
	void ShowInputError(const FString& Message) const;
};
